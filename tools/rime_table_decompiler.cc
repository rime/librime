// rime_table_decompiler.cc
// nopdan <me@nopdan.com>
//
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <rime/dict/table.h>

using namespace std;
ofstream fout;

void outCode(rime::Table* table, const rime::Code code, ofstream& fout) {
  if (code.empty()) {
    return;
  }
  auto item = code.begin();
  fout << table->GetSyllableById(*item);
  item++;
  for (; item != code.end(); ++item) {
    fout << " ";
    fout << table->GetSyllableById(*item);
  }
  return;
}

void access(rime::Table* table, rime::TableAccessor accessor) {
  while (!accessor.exhausted()) {
    auto word = table->GetEntryText(*accessor.entry());
    fout << word << "\t";
    outCode(table, accessor.code(), fout);

    auto weight = accessor.entry()->weight;
    if (weight >= 0) {
      fout << "\t" << exp(weight);
    }
    fout << endl;
    accessor.Next();
  }
}

// 递归遍历
void recursion(rime::Table* table, ofstream& fout, rime::TableQuery* query) {
  for (int i = 0; i < table->metadata()->num_syllables; i++) {
    auto accessor = query->Access(i);
    access(table, accessor);
    if (query->Advance(i)) {
      if (query->level() < 3) {
        recursion(table, fout, query);
      } else {
        auto accessor = query->Access(0);
        access(table, accessor);
      }
      query->Backdate();
    }
  }
}

void traversal(rime::Table* table, ofstream& fout) {
  auto metadata = table->metadata();
  cout << "num_syllables: " << metadata->num_syllables << endl;
  cout << "num_entries: " << metadata->num_entries << endl;

  fout << fixed;
  fout << setprecision(0);
  rime::TableQuery query(table->metadata()->index.get());
  recursion(table, fout, &query);
}

int main(int argc, char* argv[]) {
  string fileName(argv[1]);

  cout << "Read File: " << fileName << endl;
  rime::Table table(fileName);
  table.Load();

  // Remove directory if present.
  // Do this before extension removal incase directory has a period character.
  const size_t last_slash_idx = fileName.find_last_of("\\/");
  if (std::string::npos != last_slash_idx) {
    fileName.erase(0, last_slash_idx + 1);
  }

  // Remove extension if present.
  const size_t period_idx = fileName.find('.');
  if (std::string::npos != period_idx) {
    fileName.erase(period_idx);
  }

  string outputName = fileName + ".txt";
  fout.open(outputName);
  // clang-format off
  fout << "# Rime dictionary\n\n";
  fout << "---\n"
          "name: " << fileName << "\n"
          "version: \"1.0\"\n"
          "...\n\n";
  // clang-format on
  traversal(&table, fout);
  cout << "Save To: " << outputName << endl
       << endl;
  fout.close();
  return 0;
}
