#include "./WordIndex.h"

namespace searchserver {

WordIndex::WordIndex() {
  // TODO: implement  
  num_words_ = 0;
}
  
size_t WordIndex::num_words() {
  // TODO: implement
  return num_words_;
}

void WordIndex::record(const string& word, const string& doc_name) {
  // TODO: implement
  auto word_iter = words_documents_.find(word);

  if (word_iter != words_documents_.end()) {
    // if word exist
    map<string, int>& document_count = word_iter -> second;  // get the document_count pair
    auto document_iter = document_count.find(doc_name);
    if (document_iter != document_count.end()) {
      // if document exist, update rank
      int old_rank = document_iter -> second;
      document_count[doc_name] = old_rank + 1; 
    } else {
      // if document doesn't exist, create a pair for it
      document_count.insert(pair<string, int>(doc_name, 1));
    }
  } else {
    num_words_ ++; // update the unique words count
    // create a document_count pair
    map<string, int> new_document_count;  
    new_document_count.insert(pair<string, int>(doc_name, 1));
    // create a word_document pair
    words_documents_.insert(pair<string, map<string, int>>(word, new_document_count));
  }
}

vector<Result> WordIndex::lookup_word(const string& word) {
  vector<Result> result;
  // TODO: implement
  auto word_iter = words_documents_.find(word);
  if (word_iter != words_documents_.end()) {
    // if the word exist
    map<string, int> document_count = word_iter -> second;  // get the document_count pair
    auto iter = document_count.begin();
    while(iter != document_count.end()) {
      string doc_name = iter -> first;
      int rank = iter -> second;
      Result new_result = Result(doc_name, rank);
      result.push_back(new_result);
      iter++;
    }    
  }
  
  sort(result.begin(), result.end());

  return result;
}

vector<Result> WordIndex::lookup_query(const vector<string>& query) {
  
  vector<Result> results;

  // TODO: implement
  map<string, int> all_exist;  // create a map to record whether it contains all words in query
  map<string, int> document_total_counts;  // create a map to store the (document,total number) pair

  // check whether it's a valid query
  if (query.size() == 0) {
    return results;
  }

  // go through the query
  for (auto& word: query) {
    auto word_iter = words_documents_.find(word);
    if (word_iter != words_documents_.end()) {
      // if the word exist
      map<string, int> document_count = word_iter -> second;  // get the document_count pair
      auto iter = document_count.begin();
      while(iter != document_count.end()) {
        string doc_name = iter -> first;
        int rank = iter -> second;
        auto count_iter = document_total_counts.find(doc_name);
        if (count_iter != document_total_counts.end()) {
          // if the documnet exist
          count_iter -> second += rank;
          all_exist[doc_name] += 1;
        } else {
          document_total_counts.insert(pair<string, int>(doc_name, rank));
          all_exist.insert(pair<string, int>(doc_name, 1));
        }
        iter++;
      }    
    }
  }

  int query_size = query.size();

  for (auto& document: document_total_counts) {
    string doc_name = document.first;
    int rank = document.second;
    
    if(all_exist[doc_name] == query_size) {
      Result new_result = Result(doc_name, rank);
      results.push_back(new_result);
    }    
  }

  sort(results.begin(), results.end());

  return results;
}


// test print function
// void WordIndex::print_map(){
//     for(auto & iter: words_documents_) {
//         string word = iter.first;        
//         for (auto & iter2: iter.second) {
//             string doc_name = iter2.first;
//             int rank = iter2.second;
//             std::cout << "word:  " << word << "  doc:  " << doc_name << "  rank:  " << rank << std::endl;
//         }
//     }
// }

}  // namespace searchserver
