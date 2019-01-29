/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2013-10-21  GONG Chen <chen.sst@gmail.com>
 */
#ifndef RIME_LEVERS_API_H_
#define RIME_LEVERS_API_H_

#include "rime_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { char placeholder; } RimeCustomSettings;

typedef struct { char placeholder; } RimeSwitcherSettings;

typedef struct { char placeholder; } RimeSchemaInfo;

typedef struct {
  void* ptr;
  size_t i;
} RimeUserDictIterator;

typedef struct rime_levers_api_t {
  int data_size;

  RimeCustomSettings* (*custom_settings_init)(const char* config_id,
                                              const char* generator_id);
  void (*custom_settings_destroy)(RimeCustomSettings* settings);
  Bool (*load_settings)(RimeCustomSettings* settings);
  Bool (*save_settings)(RimeCustomSettings* settings);
  Bool (*customize_bool)(RimeCustomSettings* settings, const char* key, Bool value);
  Bool (*customize_int)(RimeCustomSettings* settings, const char* key, int value);
  Bool (*customize_double)(RimeCustomSettings* settings, const char* key, double value);
  Bool (*customize_string)(RimeCustomSettings* settings, const char* key, const char* value);
  Bool (*is_first_run)(RimeCustomSettings* settings);
  Bool (*settings_is_modified)(RimeCustomSettings* settings);
  Bool (*settings_get_config)(RimeCustomSettings* settings, RimeConfig* config);

  RimeSwitcherSettings* (*switcher_settings_init)();
  Bool (*get_available_schema_list)(RimeSwitcherSettings* settings,
                                    RimeSchemaList* list);
  Bool (*get_selected_schema_list)(RimeSwitcherSettings* settings,
                                   RimeSchemaList* list);
  void (*schema_list_destroy)(RimeSchemaList* list);
  const char* (*get_schema_id)(RimeSchemaInfo* info);
  const char* (*get_schema_name)(RimeSchemaInfo* info);
  const char* (*get_schema_version)(RimeSchemaInfo* info);
  const char* (*get_schema_author)(RimeSchemaInfo* info);
  const char* (*get_schema_description)(RimeSchemaInfo* info);
  const char* (*get_schema_file_path)(RimeSchemaInfo* info);
  Bool (*select_schemas)(RimeSwitcherSettings* settings,
                         const char* schema_id_list[],
                         int count);
  const char* (*get_hotkeys)(RimeSwitcherSettings* settings);
  Bool (*set_hotkeys)(RimeSwitcherSettings* settings, const char* hotkeys);

  Bool (*user_dict_iterator_init)(RimeUserDictIterator* iter);
  void (*user_dict_iterator_destroy)(RimeUserDictIterator* iter);
  const char* (*next_user_dict)(RimeUserDictIterator* iter);
  Bool (*backup_user_dict)(const char* dict_name);
  Bool (*restore_user_dict)(const char* snapshot_file);
  int (*export_user_dict)(const char* dict_name, const char* text_file);
  int (*import_user_dict)(const char* dict_name, const char* text_file);

  // patch a list or a map
  Bool (*customize_item)(RimeCustomSettings* settings,
                         const char* key, RimeConfig* value);

} RimeLeversApi;

#ifdef __cplusplus
}
#endif

#endif  // RIME_LEVERS_API_H_
