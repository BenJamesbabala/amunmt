#include <vector>
#include <sstream>
#include <boost/range/adaptor/map.hpp>

#include <yaml-cpp/yaml.h>

#include "common/processor/bpe.h"
#include "common/file_stream.h"
#include "common/filter.h"
#include "common/processor/processor.h"
#include "common/threadpool.h"
#include "common/vocab.h"

#include "decoder/config.h"
#include "decoder/god.h"
#include "decoder/loader_factory.h"
#include "decoder/scorer.h"

God God::instance_;

God& God::Init(int argc, char** argv) {
  return Summon().NonStaticInit(argc, argv);
}

God& God::Init(const std::string& options) {
  std::vector<std::string> args = boost::program_options::split_unix(options);
  int argc = args.size() + 1;
  char* argv[argc];
  argv[0] = const_cast<char*>("bogus");
  for(int i = 1; i < argc; i++)
    argv[i] = const_cast<char*>(args[i-1].c_str());
  return Init(argc, argv);
}

God& God::NonStaticInit(int argc, char** argv) {
  info_ = spdlog::stderr_logger_mt("info");
  info_->set_pattern("[%c] (%L) %v");

  progress_ = spdlog::stderr_logger_mt("progress");
  progress_->set_pattern("%v");

  config_.AddOptions(argc, argv);
  config_.LogOptions();

  if(Get("source-vocab").IsSequence()) {
    for(auto sourceVocabPath : Get<std::vector<std::string>>("source-vocab"))
      sourceVocabs_.emplace_back(new Vocab(sourceVocabPath));
  }
  else {
    sourceVocabs_.emplace_back(new Vocab(Get<std::string>("source-vocab")));
  }
  targetVocab_.reset(new Vocab(Get<std::string>("target-vocab")));

  weights_ = Get<std::map<std::string, float>>("weights");

  if(Get<bool>("show-weights")) {
    LOG(info) << "Outputting weights and exiting";
    for(auto && pair : weights_) {
      std::cout << pair.first << "= " << pair.second << std::endl;
    }
    exit(0);
  }

  for(auto&& pair : config_.Get()["scorers"]) {
    std::string name = pair.first.as<std::string>();
    loaders_.emplace(name, LoaderFactory::Create(name, pair.second));
  }

  if (!Get<std::vector<std::string>>("softmax-filter").empty()) {
    auto filterOptions = Get<std::vector<std::string>>("softmax-filter");
    std::string alignmentFile = filterOptions[0];
    LOG(info) << "Reading target softmax filter file from " << alignmentFile;
    Filter* filter = nullptr;
    if (filterOptions.size() >= 3) {
      const size_t numNFirst = stoi(filterOptions[1]);
      const size_t maxNumTranslation = stoi(filterOptions[2]);
      filter = new Filter(GetSourceVocab(0),
                          GetTargetVocab(),
                          alignmentFile,
                          numNFirst,
                          maxNumTranslation);
    } else if (filterOptions.size() == 2) {
      const size_t numNFirst = stoi(filterOptions[1]);
      filter = new Filter(GetSourceVocab(0),
                          GetTargetVocab(),
                          alignmentFile,
                          numNFirst);
    } else {
      filter = new Filter(GetSourceVocab(0),
                          GetTargetVocab(),
                          alignmentFile);
    }
    filter_.reset(filter);
  }

  if (Has("input-file")) {
    LOG(info) << "Reading from " << Get<std::string>("input-file");
    inputStream_.reset(new InputFileStream(Get<std::string>("input-file")));
  }
  else {
    LOG(info) << "Reading from stdin";
    inputStream_.reset(new InputFileStream(std::cin));
  }

  if (Get<std::string>("bpe") != "") {
    LOG(info) << "Using BPE from: " << Get<std::string>("bpe");
    processors_.emplace_back(new BPE(Get<std::string>("bpe")));
  }

  return *this;
}

Vocab& God::GetSourceVocab(size_t i) {
  return *(Summon().sourceVocabs_[i]);
}

Vocab& God::GetTargetVocab() {
  return *Summon().targetVocab_;
}

Filter& God::GetFilter() {
  return *(Summon().filter_);
}

std::istream& God::GetInputStream() {
  return *Summon().inputStream_;
}

std::vector<ScorerPtr> God::GetScorers(size_t taskId) {
  std::vector<ScorerPtr> scorers;
  for(auto&& loader : Summon().loaders_ | boost::adaptors::map_values)
    scorers.emplace_back(loader->NewScorer(taskId));
  return scorers;
}

std::vector<std::string> God::GetScorerNames() {
  std::vector<std::string> scorerNames;
  for(auto&& name : Summon().loaders_ | boost::adaptors::map_keys)
    scorerNames.push_back(name);
  return scorerNames;
}

std::map<std::string, float>& God::GetScorerWeights() {
  return Summon().weights_;
}

// clean up cuda vectors before cuda context goes out of scope
void God::CleanUp() {
  for(auto& loader : Summon().loaders_ | boost::adaptors::map_values)
    loader.reset(nullptr);
}

std::vector<std::string> God::Preprocess(const std::vector<std::string>& input) {
  std::vector<std::string> processed = input;
  for (const auto& processor : Summon().processors_) {
    processed = processor->Preprocess(processed);
  }
  return processed;
}

std::vector<std::string> God::Postprocess(const std::vector<std::string>& input) {
  std::vector<std::string> processed = input;
  for (auto processor = Summon().processors_.rbegin(); processor != Summon().processors_.rend(); ++processor) {
    processed = (*processor)->Postprocess(processed);
  }
  return processed;
}


