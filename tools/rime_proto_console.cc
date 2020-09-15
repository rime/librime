/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2011-08-29 GONG Chen <chen.sst@gmail.com>
 */
#include <algorithm>
#include <iostream>
#include <string>
#include <capnp/message.h>
#include <rime_api.h>
#include <rime_message.capnp.h>
#include <rime_proto.capnp.h>

void print_status(rime::proto::Status::Reader status) {
  std::cout << "schema: " << status.getSchemaId().cStr()
            << " / " << status.getSchemaName().cStr() << std::endl;
  std::cout << "status:";
  if (status.getIsDisabled()) std::cout << " disabled";
  if (status.getIsComposing()) std::cout << " composing";
  if (status.getIsAsciiMode()) std::cout << " ascii";
  if (status.getIsFullShape()) std::cout << " full_shape";
  if (status.getIsSimplified()) std::cout << " simplified";
  std::cout << std::endl;
}

void print_composition(rime::proto::Context::Composition::Reader composition) {
  if (!composition.hasPreedit()) return;
  std::string preedit = composition.getPreedit();
  size_t len = composition.getLength();
  size_t start = composition.getSelStart();
  size_t end = composition.getSelEnd();
  size_t cursor = composition.getCursorPos();
  for (size_t i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) {
        std::cout << '[';
      } else if (i == end) {
        std::cout << ']';
      }
    }
    if (i == cursor) std::cout << '|';
    if (i < len)
      std::cout << preedit[i];
  }
  std::cout << std::endl;
}

void print_menu(rime::proto::Context::Menu::Reader menu) {
  int num_candidates = menu.getCandidates().size();
  if (num_candidates == 0) return;
  std::cout << "page: " << (menu.getPageNumber() + 1)
            << (menu.getIsLastPage() ? '$' : ' ')
            << " (of size " << menu.getPageSize() << ")" << std::endl;
  for (int i = 0; i < num_candidates; ++i) {
    bool highlighted = i == menu.getHighlightedCandidateIndex();
    auto candidate = menu.getCandidates()[i];
    std::cout << (i + 1) << ". "
              << (highlighted ? '[' : ' ')
              << candidate.getText().cStr()
              << (highlighted ? ']' : ' ');
    if (candidate.hasComment())
      std::cout << candidate.getComment().cStr();
    std::cout << std::endl;
  }
}

void print_context(rime::proto::Context::Reader context) {
  auto composition = context.getComposition();
  if (composition.getLength() > 0) {
    print_composition(composition);
    print_menu(context.getMenu());
  } else {
    std::cout << "(not composing)" << std::endl;
  }
}

void print(RimeSessionId session_id) {
  RimeApi* rime = rime_get_api();

  ::capnp::MallocMessageBuilder message_builder;
  RimeMessage::Builder root = message_builder.initRoot<RimeMessage>();

  auto commit_builder = root.getCommit();
  rime->commit_proto(session_id, &commit_builder);
  auto commit_reader = commit_builder.asReader();
  if (commit_reader.hasText()) {
    std::cout << "commit: " << commit_reader.getText().cStr() << std::endl;
  }

  auto status_builder = root.getStatus();
  rime->status_proto(session_id, &status_builder);
  print_status(status_builder.asReader());

  auto context_builder = root.getContext();
  rime->context_proto(session_id, &context_builder);
  print_context(context_builder.asReader());
}

inline static bool is_prefix_of(const std::string& str,
                                const std::string& prefix) {
  return std::mismatch(prefix.begin(),
                       prefix.end(),
                       str.begin()).first == prefix.end();
}

bool execute_special_command(const std::string& line,
                             RimeSessionId session_id) {
  RimeApi* rime = rime_get_api();
  if (line == "print schema list") {
    RimeSchemaList list;
    if (rime->get_schema_list(&list)) {
      std::cout << "schema list:" << std::endl;
      for (size_t i = 0; i < list.size; ++i) {
        std::cout << (i + 1) << ". " << list.list[i].name
                  << " [" << list.list[i].schema_id << "]" << std::endl;
      }
      rime->free_schema_list(&list);
    }
    char current[100] = {0};
    if (rime->get_current_schema(session_id, current, sizeof(current))) {
      std::cout << "current schema: [" << current << "]" << std::endl;
    }
    return true;
  }
  const std::string kSelectSchema = "select schema ";
  if (is_prefix_of(line, kSelectSchema)) {
    auto schema_id = line.substr(kSelectSchema.length());
    if (rime->select_schema(session_id, schema_id.c_str())) {
      std::cout << "selected schema: [" << schema_id << "]" << std::endl;
    }
    return true;
  }
  const std::string kSelectCandidate = "select candidate ";
  if (is_prefix_of(line, kSelectCandidate)) {
    int index = std::stoi(line.substr(kSelectCandidate.length()));
    if (index > 0 &&
        rime->select_candidate_on_current_page(session_id, index - 1)) {
      print(session_id);
    } else {
      std::cerr << "cannot select candidate at index " << index << "."
                << std::endl;
    }
    return true;
  }
  if (line == "print candidate list") {
    RimeCandidateListIterator iterator = {0};
    if (rime->candidate_list_begin(session_id, &iterator)) {
      while (rime->candidate_list_next(&iterator)) {
        std::cout << (iterator.index + 1) << ". " << iterator.candidate.text;
        if (iterator.candidate.comment)
          std::cout << " (" << iterator.candidate.comment << ")";
        std::cout << std::endl;
      }
      rime->candidate_list_end(&iterator);
    } else {
      std::cout << "no candidates." << std::endl;
    }
    return true;
  }
  const std::string kSetOption = "set option ";
  if (is_prefix_of(line, kSetOption)) {
    Bool is_on = True;
    auto iter = line.begin() + kSetOption.length();
    const auto end = line.end();
    if (iter != end && *iter == '!') {
      is_on = False;
      ++iter;
    }
    auto option = std::string(iter, end);
    rime->set_option(session_id, option.c_str(), is_on);
    std::cout << option << " set " << (is_on ? "on" : "off") << std::endl;
    return true;
  }
  return false;
}

void on_message(void* context_object,
                RimeSessionId session_id,
                const char* message_type,
                const char* message_value) {
  std::cout << "message: [" << session_id << "] [" << message_type << "] "
            << message_value << std::endl;
}

int main(int argc, char *argv[]) {
  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "rime.console";
  rime->setup(&traits);

  rime->set_notification_handler(&on_message, NULL);

  std::cerr << "initializing..." << std::endl;
  rime->initialize(NULL);
  Bool full_check = True;
  if (rime->start_maintenance(full_check))
    rime->join_maintenance_thread();
  std::cerr << "ready." << std::endl;

  RimeSessionId session_id = rime->create_session();
  if (!session_id) {
    std::cerr << "Error creating rime session." << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "exit")
      break;
    if (execute_special_command(line, session_id))
      continue;
    if (rime->simulate_key_sequence(session_id, line.c_str())) {
      print(session_id);
    } else {
      std::cerr << "Error processing key sequence: " << line << std::endl;
    }
  }

  rime->destroy_session(session_id);
  rime->finalize();
  return 0;
}
