#include<bits/stdc++.h>
using namespace std;

std::vector<std::string> split_str(std::string src, std::string split_word) {
  std::vector<std::string> answers;
  int len = (int) split_word.size();

  std::string word_so_far = "";

  for (int i = 0; i < src.size(); i++) {
    word_so_far += src[i];

    if (i + len - 1 > src.size()) continue;

    std::string temp = src.substr(i, len);
    if (temp == split_word) {
      if (!word_so_far.empty()) word_so_far.pop_back();
      answers.push_back(word_so_far);
      word_so_far = "";
      i = i + len - 1;
    }
  }

  if (!word_so_far.empty()) answers.push_back(word_so_far);
  return answers;
}

int main() {
    string d = "hi, df, d, f, d, f";
    vector<string> f = split_str(d, ", ");

    for (auto x: f) {
        cout << x << " ";
    }
    cout << "\n";
}