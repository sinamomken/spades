/*
 * read.hpp
 *
 *  Created on: 29.03.2011
 *      Author: vyahhi
 */

#ifndef READ_HPP_
#define READ_HPP_

#include "quality.hpp"
#include "sequence.hpp"
#include "nucl.hpp"
#include <string>
using namespace std;

class Read {
public:
	const bool isValid() const {
		if (seq_.size() == 0) {
			return false;
		}
		for (size_t i = 0; i < seq_.size(); ++i) {
			if (!is_nucl(seq_[i])) {
				return false;
			}
		}
		return true;
	}

	Sequence* createSequence() const {
		return new Sequence(seq_);
	}

	Quality* createQuality() const {
		return new Quality(qual_);
	}

	const string& getSequence() const {
		return seq_;
	}
	const string& getQuality() const{
		return qual_;
	}
	const string& getName() const {
		return name_;
	}
	size_t size() const {
		return seq_.size();
	}
	char operator[](size_t i) const {
		assert(is_nucl(seq_[i]));
		return dignucl(seq_[i]);
	}
private:
	static const int PHRED_OFFSET = 33;
	string seq_;
	string qual_;
	string name_;
	friend class ireadstream;
	void setName(const char* s) {
		name_ = s;
	}
	void setQuality(const char* s) {
		qual_ = s;
		for (size_t i = 0; i < qual_.size(); ++i) {
			qual_[i] -= PHRED_OFFSET;
		}
	}
	void setSequence(const char* s) {
		seq_ = s;
	}
};

#endif /* READ_HPP_ */
