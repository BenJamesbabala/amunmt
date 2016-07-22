#include "decoder/hypothesis.h"

#include "decoder/god.h"
#include "common/vocab.h"

std::ostream& operator<<(std::ostream &out, const Hypothesis &obj)
{
	Vocab &vocab = God::GetTargetVocab();
	out << "WORD:" << obj.word_ << "(" << vocab[obj.word_] << ")";
	out << " COST:" << obj.cost_ << " = ";
	for (auto cost : obj.costBreakdown_) {
		out << cost << " ";
	}
	return out;
}

