#pragma once

#include "compare_standard.hpp"
#include "longseq.hpp"
//#include "longseq_storage.hpp"
#include "polynomial_hash.hpp" 
#include "debruijn_kmer_index.hpp"

template<>
struct kmer_index_traits<cap::LSeq> {
  typedef cap::LSeq SeqType;
  // FIXME: Provide implementation here
  typedef std::vector<cap::LSeq> RawKMerStorage;

  typedef RawKMerStorage::iterator             raw_data_iterator;
  typedef RawKMerStorage::const_iterator       raw_data_const_iterator;
  typedef RawKMerStorage::iterator::value_type KMerRawData;
  typedef RawKMerStorage::iterator::reference  KMerRawReference;

  struct raw_equal_to {
    inline bool operator()(const SeqType &lhs, const KMerRawReference rhs) {
      // Using fast_equal_to, which relies only on hash:
      // 1. True comparison leads to poor performance on large k
      // 2. Hashes are to be different (in other case MPH is impossible)
      return SeqType::fast_equal_to()(lhs, rhs);
    }
  };

  struct hash_function {
    inline uint64_t operator()(const SeqType &k) const {
      return k.GetHash().get<2>();
    }
    inline uint64_t operator()(const KMerRawReference k) const {
      return k.GetHash().get<2>();
    }
  };

  struct seeded_hash_function {
    static cxxmph::h128 hash128(const KMerRawReference k, uint32_t seed) {
      SeqType::HashType hash = k.GetHash();
//      uint64_t salt = hash.get<2>();
      cxxmph::h128 h;
      MurmurHash3_x64_128(reinterpret_cast<const void*>(&hash), sizeof(hash),
          seed, &h);
//      h.set64(hash.get<0>(), 0);
//      h.set64(hash.get<1>(), 1);

//      INFO("SEE MAN:: hash=" << hash.get<0>() << ", seed=" << seed << ", result = " << h.get64(0) << " " << h.get64(1) << " " << k.str());
      return h;
    }

    static cxxmph::h128 hash128(const SeqType &k, uint32_t seed) {
      SeqType::HashType hash = k.GetHash();
//      uint64_t salt = hash.get<2>();
      cxxmph::h128 h;
      MurmurHash3_x64_128(reinterpret_cast<const void*>(&hash), sizeof(hash),
          seed, &h);
//      h.set64(hash.get<0>(), 0);
//      h.set64(hash.get<1>(), 1);

//      INFO("SEE MAN:: hash=" << hash.get<0>() << ", seed=" << seed << ", result = " << h.get64(0) << " " << h.get64(1) << " " << k.str());

      return h;
    }
  };

  struct raw_create {
    inline SeqType operator()(unsigned K, const KMerRawReference kmer) {
      return SeqType(kmer);
    }
  };
};

namespace cap {

struct Foo {};  

class CapKMerIndexBuilder;
  
/*
class CapKMerIndex: public debruijn_graph::DeBruijnKMerIndex<Foo,
                                      cap::LongSeq<DoublePolynomialHash<> > > {
  friend class CapKMerIndexBuilder;
};
*/
// FIXME: Implement stuff here
template <class Read>
class CapKMerCounter: public ::KMerCounter<LSeq> {
  typedef KMerCounter<LSeq> __super;
  typedef typename __super::RawKMerStorage RawKMerStorage;

  unsigned k_;
  io::ReadStreamVector<io::IReader<Read> > &streams_;
  std::unordered_set<LSeq, LSeq::hash, LSeq::equal_to> storage_;
  RawKMerStorage *bucket;
  
  /*
  void UpdateTransition(const LSeq &l, const LSeq &r) {
    const LSeq &l_int = LongSeqStorage<LSeq>::Instance().Get(l),
               &r_int = LongSeqStorage<LSeq>::Instance().Get(r);
    VERIFY(l.GetNextNucl() != LSeq::kNoNextNucl);
    l_int.UpdateTransition(l.GetNextNucl(), &r_int);
  }
  */

 public:
  CapKMerCounter(unsigned k, io::ReadStreamVector<io::IReader<Read> > &streams)
      : k_(k),
        streams_(streams),
        storage_(),
        bucket(NULL) {
    TRACE("Creating LSEQ counter");
    //LongSeqStorage<LSeq>::Instance().clear();
    for (size_t i = 0; i < streams_.size(); ++i) {
      while (!streams_[i].eof()) {
        Read r;
        streams_[i] >> r;
        const Sequence &seq = r.sequence();
        if (seq.size() == 0) {
          continue;
        }
        if (seq.size() < k) {
          INFO("WARNING: too small sequence!!");
          continue;
        }

        LSeq kmer(k, seq);
        do {
          storage_.insert(kmer);
          //LongSeqStorage<LSeq>::Instance().Put(kmer);
          kmer.Shift();
        } while (kmer.IsValid());

        // Build transitions between kmers
        /*
        kmer = LSeq(k, seq);
        LSeq next_kmer(k, seq, 1);
        do {
          UpdateTransition(kmer, next_kmer);
          kmer.Shift();
          next_kmer.Shift();
        } while (next_kmer.IsValid());
        */
      }
    }

    TRACE("End creating LSEQ counter");
  }

  virtual ~CapKMerCounter() {
    ReleaseBucket(0);
  }

  virtual size_t Count(unsigned num_buckets, unsigned num_threads) {
    INFO("K-mer counting done. There are " << storage_.size() << " kmers in total. ");
    return storage_.size();
  }

  virtual void MergeBuckets(unsigned num_buckets) {
    VERIFY(bucket == NULL);

    TRACE("MERGING BUckets");
    OpenBucket(0); // great logic
  }

  virtual void OpenBucket(size_t idx, bool unlink = true) {
    VERIFY(bucket == NULL);

    TRACE("BUCKET OPEN");
    bucket = new RawKMerStorage();
    bucket->reserve(storage_.size());
    for (auto it = storage_.begin(); it != storage_.end(); ++it) {
      bucket->push_back(*it);
    }
  }

  virtual void ReleaseBucket(size_t idx) {
    TRACE("RELEASE BUCKET");
    delete bucket;
    bucket = NULL;
  }

  virtual RawKMerStorage* TransferBucket(size_t idx) {
    VERIFY(bucket != NULL);
    TRACE("TRANSFERRING BUCKET" <<
      "BUCKET size=" << bucket->size());

    RawKMerStorage *ret = bucket;
    bucket = NULL;

    return ret;
  }

  virtual typename __super::iterator bucket_begin(size_t idx) {
    return bucket->begin();
  }
  virtual typename __super::iterator bucket_end(size_t idx) {
    return bucket->end();
  }

};

}

namespace debruijn_graph {

template<>
class DeBruijnKMerIndexBuilder<cap::LSeq> {

  template <class ReadStream, class IdType>
  size_t FillCoverageFromStream(ReadStream &stream,
                                DeBruijnKMerIndex<IdType, cap::LSeq> &index) const {
    unsigned K = index.K();
    size_t rl = 0;

    while (!stream.eof()) {
      typename ReadStream::read_type r;
      stream >> r;
      rl = std::max(rl, r.size());

      const Sequence &seq = r.sequence();
      if (seq.size() < K)
        continue;

      cap::LSeq kmer = seq.start<cap::LSeq>(K);

      size_t idx = index.seq_idx(kmer);
      VERIFY(index.contains(idx, kmer));
#   pragma omp atomic
      index.data_[idx].count_ += 1;
      for (size_t j = K; j < seq.size(); ++j) {
        kmer.Shift();
        idx = index.seq_idx(kmer);
        VERIFY(index.contains(idx, kmer));

#     pragma omp atomic
        index.data_[idx].count_ += 1;
      }
    }

    return rl;
  }

 public:
  template <class Read, class IdType>
  size_t BuildIndexFromStream(
      DeBruijnKMerIndex<IdType, cap::LSeq> &index,
      io::ReadStreamVector<io::IReader<Read> > &streams,
      SingleReadStream* contigs_stream = 0) {
    cap::CapKMerCounter<Read> counter(index.K(), streams);
    KMerIndexBuilder<typename DeBruijnKMerIndex<IdType, cap::LSeq>::KMerIndexT> builder(index.workdir(),
                                                                1 /* buckets */,
                                                                1 /* threads */);
    size_t sz = builder.BuildIndex(index.index_, counter, /* save final */ true);

    SortUniqueKMers(counter, index);

    // Now use the index to fill the coverage and EdgeId's
    INFO("Collecting k-mer coverage information, this takes a while.");
    index.data_.resize(sz);

    // Fill coverage (do we need to?)
    size_t rl = 0;
    streams.reset();
    unsigned nthreads = streams.size();
# pragma omp parallel for num_threads(nthreads) shared(rl)
    for (size_t i = 0; i < nthreads; ++i) {
      size_t crl = FillCoverageFromStream(streams[i], index);

      // There is no max reduction in C/C++ OpenMP... Only in FORTRAN :(
#   pragma omp flush(rl)
      if (crl > rl)
#     pragma omp critical
      {
        rl = std::max(rl, crl);
      }
    }

    if (contigs_stream) {
      contigs_stream->reset();
      FillCoverageFromStream(*contigs_stream, index);
    }

    return rl;
  }

 protected:
  template <class KMerCounter, class Index>
  void SortUniqueKMers(KMerCounter &counter, Index &index) const {
    if (!index.kmers)
      index.kmers = counter.TransferBucket(0);

    size_t swaps = 0;
    INFO("Arranging kmers in hash map order");
    for (auto I = index.kmers->begin(), E = index.kmers->end(); I != E; ++I) {
      size_t cidx = I - index.kmers->begin();
      size_t kidx = index.raw_seq_idx(*I);
      while (cidx != kidx) {
        auto J = index.kmers->begin() + kidx;
        using std::swap;
        swap(*I, *J);
        swaps += 1;

        kidx = index.raw_seq_idx(*I);
      }
    }
    INFO("Done. Total swaps: " << swaps);
  }

 protected:
  DECL_LOGGER("K-mer Index Building");
};


}


namespace runtime_k {


template <typename Value>
class KmerMap<Value, cap::LSeq> {
 public:
  typedef typename cap::LSeq key_type;
  typedef typename std::pair<const key_type, Value> value_type;
  typedef KmerMap<Value, cap::LSeq> map_type;

 private:
  // Note using equal_to which maintains special 'transitions' inside LongSeqs
  typedef std::unordered_map<key_type, Value, typename key_type::hash, typename key_type::equal_to> int_map_type;
  typedef typename std::pair<const key_type, const Value> const_value_type;
  int_map_type *data_;

  class InnerIterator {
    friend class KmerMap<Value, cap::LSeq>;
    typedef typename int_map_type::iterator base;
    base base_;

   public:

    InnerIterator(const base &iter): base_(iter) {
    }

    InnerIterator &operator++() {
      ++base_;
      return *this;
    }
    InnerIterator operator++(int) {
      InnerIterator stored = *this;
      ++base_;
      return stored;
    }

    value_type operator*() {
        return base_->operator *();
    }

    const key_type first() {
        return base_->first();
    }

    Value& second() {
        return base_->second();
    }

    bool operator==(const InnerIterator& iter) const {
        return base_ == iter.base_;
    }
    bool operator!=(const InnerIterator& iter) const {
      return !operator==(iter);
    }

  };

  class InnerConstIterator {
    friend class KmerMap<Value, cap::LSeq>;
    typedef typename int_map_type::const_iterator base;
    base base_;

   public:

    InnerConstIterator(const base &iter): base_(iter) {
    }

    InnerConstIterator &operator++() {
      ++base_;
      return *this;
    }
    InnerConstIterator operator++(int) {
      InnerConstIterator stored = *this;
      ++base_;
      return stored;
    }

    const value_type operator*() const {
        return base_->operator *();
    }

    key_type first() const {
        return base_->first;
    }

    const Value& second() const {
        return base_->second;
    }

    bool operator==(const InnerConstIterator& iter) const {
        return base_ == iter.base_;
    }
    bool operator!=(const InnerConstIterator& iter) const {
      return !operator==(iter);
    }

  };

 public:
  typedef InnerIterator iterator;
  typedef InnerConstIterator const_iterator;

  KmerMap(size_t k) {
    data_ = new int_map_type();
  }

  KmerMap(int_map_type *map) : data_(map) {
  }

  KmerMap(const map_type& map) {
    data_ = new int_map_type(*map.data);
  }

  map_type& operator=(const map_type& map) {
    if (map.data_ != data_) {
      delete data_;
      data_ = new int_map_type(*map.data_);
    }

    return *this;
  }

  ~KmerMap() {
    delete data_;
  }

  bool empty() const {
    return data_->empty();
  }

  size_t size() const {
    return data_->size();
  }

  size_t max_size() const {
    return data_->max_size();
  }

  const_iterator begin() const {
    return InnerConstIterator(data_->begin());
  }

  iterator begin() {
    return InnerIterator(data_->begin());
  }

  const_iterator end() const {
    return InnerConstIterator(data_->end());
  }

  iterator end() {
    return InnerIterator(data_->end());
  }

  Value& operator[](const key_type& kmer_seq) {
    return data_->operator [](kmer_seq);
  }

  const_iterator find(const key_type& kmer_seq) const {
    return InnerConstIterator(data_->find(kmer_seq));
  }

  iterator find(const key_type& kmer_seq) {
    return InnerIterator(data_->find(kmer_seq));
  }

  size_t count(const key_type& kmer_seq) const {
    return data_->count(kmer_seq);
  }

  pair<iterator, bool> insert(const value_type& val) {
    return data_->insert(val);
  }

  size_t erase(const key_type& kmer_seq) {
    return data_->erase(kmer_seq);
  }

  //    iterator erase(const const_iterator& iter) {
  //        return iterator(data_->erase(iter.get_data()));
  //    }

  iterator erase(const iterator& iter) {
    return data_->erase(iter.base_);
  }

  void clear() {
    data_->clear();
  }

  /*
  size_t bucket_count() const {
    return data_->bucket_count();
  }

  size_t max_bucket_count() const {
    return data_->max_bucket_count();
  }

  size_t bucket_size(size_t n) const {
    return data_->bucket_size(n);
  }

  size_t bucket(const RtSeq& kmer_seq) const {
    return data_->bucket(kmer_seq);
  }

  float load_factor() const {
    return data_->load_factor();
  }

  float max_load_factor() const {
    return data_->max_load_factor();
  }

  void max_load_factor(float z) {
    data_->max_load_factor(z);
  }

  void rehash(size_t n) {
    data_->rehash(n);
  }

  size_t get_k() const {
    return data_->get_k();
  }

  int_map_type& get_data() {
    return *data_;
  }
  */



};

};