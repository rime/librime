// rime_table_decompiler.cc
// nopdan <me@nopdan.com>
//
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <rime/dict/table.h>
#include "codepage.h"

// usage:
//   rime_table_decompiler <rime-table-file> [save-path]
// example:
//   rime_table_decompiler pinyin.table.bin pinyin.dict.yaml

void outCode(rime::Table* table, const rime::Code code, std::ofstream& fout) {
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

void access(rime::Table* table,
            rime::TableAccessor accessor,
            std::ofstream& fout) {
  while (!accessor.exhausted()) {
    auto word = table->GetEntryText(*accessor.entry());
    fout << word << "\t";
    outCode(table, accessor.code(), fout);

    auto weight = accessor.entry()->weight;
    if (weight >= 0) {
      fout << "\t" << exp(weight);
    }
    fout << std::endl;
    accessor.Next();
  }
}

// recursively traverse table
void recursion(rime::Table* table,
               rime::TableQuery* query,
               std::ofstream& fout) {
  for (int i = 0; i < table->metadata()->num_syllables; i++) {
    auto accessor = query->Access(i);
    access(table, accessor, fout);
    if (query->Advance(i)) {
      if (query->level() < 3) {
        recursion(table, query, fout);
      } else {
        auto accessor = query->Access(0);
        access(table, accessor, fout);
      }
      query->Backdate();
    }
  }
}

void traversal(rime::Table* table, std::ofstream& fout) {
  auto metadata = table->metadata();
  std::cout << "num_syllables: " << metadata->num_syllables << std::endl;
  std::cout << "num_entries: " << metadata->num_entries << std::endl;

  fout << std::fixed;
  fout << std::setprecision(0);
  rime::TableQuery query(table->metadata()->index.get());
  recursion(table, &query, fout);
}

rime::path InferredOutputPath(rime::path input_path) {
  if (input_path.extension() == ".bin") {
    input_path.replace_extension();
    if (input_path.extension() == ".table") {
      return input_path.replace_extension(".dict.yaml");
    }
  }
  return input_path.concat(".yaml");
}

int main(int argc, char* argv[]) {
  unsigned int codepage = SetConsoleOutputCodePage();
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: rime_table_decompiler <rime-table-file> [save-path]"
              << std::endl;
    std::cout
        << "Example: rime_table_decompiler pinyin.table.bin pinyin.dict.yaml"
        << std::endl;
    SetConsoleOutputCodePage(codepage);
    return 0;
  }

  rime::path file_path(argv[1]);
  rime::Table table(file_path);
  bool success = table.Load();
  if (!success) {
    std::cerr << "Failed to load table." << std::endl;
    SetConsoleOutputCodePage(codepage);
    return 1;
  }

  rime::path output_path =
      (argc == 3) ? rime::path(argv[2]) : InferredOutputPath(file_path);

  std::ofstream fout;
  fout.open(output_path.c_str());
  if (!fout.is_open()) {
    std::cerr << "Failed to open file " << output_path << std::endl;
    SetConsoleOutputCodePage(codepage);
    return 1;
  }

  // schema id
  fout << "# Rime dictionary\n\n";
  fout << "---\n"
          "name: "
       << file_path.stem().u8string()
       << "\n"
          "version: \"1.0\"\n"
          "...\n\n";
  traversal(&table, fout);
  std::cout << "Save to: " << output_path << std::endl;
  fout.close();
  SetConsoleOutputCodePage(codepage);
  return 0;
}
