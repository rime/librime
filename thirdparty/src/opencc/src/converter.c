/*
 * Open Chinese Convert
 *
 * Copyright 2010-2013 BYVoid <byvoid@byvoid.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"
#include "converter.h"
#include "dict_group.h"
#include "dict_chain.h"
#include "encoding.h"

#define DELIMITER ' '
#define SEGMENT_MAXIMUM_LENGTH 0
#define SEGMENT_SHORTEST_PATH 1
#define SEGMENT_METHOD SEGMENT_SHORTEST_PATH

#if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH
# define OPENCC_SP_SEG_DEFAULT_BUFFER_SIZE 1024
typedef struct {
  int initialized;
  size_t buffer_size;
  size_t* match_length;
  size_t* min_len;
  size_t* parent;
  size_t* path;
} SpsegData;
#endif

static converter_error errnum = CONVERTER_ERROR_VOID;

#if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH
static void sp_seg_buffer_free(SpsegData* ossb) {
  free(ossb->match_length);
  free(ossb->min_len);
  free(ossb->parent);
  free(ossb->path);
}

static void sp_seg_set_buffer_size(SpsegData* ossb, size_t buffer_size) {
  if (ossb->initialized == 1) {
    sp_seg_buffer_free(ossb);
  }
  ossb->buffer_size = buffer_size;
  ossb->match_length = (size_t*)malloc((buffer_size + 1) * sizeof(size_t));
  ossb->min_len = (size_t*)malloc(buffer_size * sizeof(size_t));
  ossb->parent = (size_t*)malloc(buffer_size * sizeof(size_t));
  ossb->path = (size_t*)malloc(buffer_size * sizeof(size_t));
  ossb->initialized = 1;
}

static size_t sp_seg(Converter* converter,
                     ucs4_t** inbuf,
                     size_t* inbuf_left,
                     ucs4_t** outbuf,
                     size_t* outbuf_left,
                     size_t length) {
  /* 最短路徑分詞 */
  /* 對長度爲1時特殊優化 */
  if (length == 1) {
    const ucs4_t* const* match_rs = dict_group_match_longest(
      converter->current_dict_group,
      *inbuf,
      1,
      NULL);
    size_t match_len = 1;
    if (converter->conversion_mode == OPENCC_CONVERSION_FAST) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        const ucs4_t* result = match_rs[0];
        /* 輸出緩衝區剩餘空間小於分詞長度 */
        if (ucs4len(result) > *outbuf_left) {
          errnum = CONVERTER_ERROR_OUTBUF;
          return (size_t)-1;
        }
        for (; *result; result++) {
          **outbuf = *result;
          (*outbuf)++, (*outbuf_left)--;
        }
        *inbuf += match_len;
        *inbuf_left -= match_len;
      }
    } else if (converter->conversion_mode ==
               OPENCC_CONVERSION_LIST_CANDIDATES) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        size_t i;
        for (i = 0; match_rs[i] != NULL; i++) {
          const ucs4_t* result = match_rs[i];
          int show_delimiter = match_rs[i + 1] != NULL ? 1 : 0;
          /* 輸出緩衝區剩餘空間小於分詞長度 */
          if (ucs4len(result) + show_delimiter > *outbuf_left) {
            errnum = CONVERTER_ERROR_OUTBUF;
            return (size_t)-1;
          }
          for (; *result; result++) {
            **outbuf = *result;
            (*outbuf)++, (*outbuf_left)--;
          }
          if (show_delimiter) {
            **outbuf = DELIMITER;
            (*outbuf)++, (*outbuf_left)--;
          }
        }
        *inbuf += match_len;
        *inbuf_left -= match_len;
      }
    } else if (converter->conversion_mode == OPENCC_CONVERSION_SEGMENT_ONLY) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        /* 輸出緩衝區剩餘空間小於分詞長度 */
        if (match_len + 1 > *outbuf_left) {
          errnum = CONVERTER_ERROR_OUTBUF;
          return (size_t)-1;
        }
        size_t i;
        for (i = 0; i < match_len; i++) {
          **outbuf = **inbuf;
          (*outbuf)++, (*outbuf_left)--;
          (*inbuf)++, (*inbuf_left)--;
        }
      }
      **outbuf = DELIMITER;
      (*outbuf)++, (*outbuf_left)--;
    } else {
      debug_should_not_be_here();
    }
    /* 必須保證有一個字符空間 */
    return match_len;
  }

  /* 設置緩衝區空間 */
  SpsegData* ossb = converter->data;
  size_t buffer_size_need = length + 1;
  if ((ossb->initialized == 0) || (ossb->buffer_size < buffer_size_need)) {
    sp_seg_set_buffer_size(ossb, buffer_size_need);
  }
  size_t i, j;
  for (i = 0; i <= length; i++) {
    ossb->min_len[i] = INFINITY_INT;
  }
  ossb->min_len[0] = ossb->parent[0] = 0;
  for (i = 0; i < length; i++) {
    /* 獲取所有匹配長度 */
    size_t match_count = dict_group_get_all_match_lengths(
      converter->current_dict_group,
      (*inbuf) + i,
      ossb->match_length
      );
    if (ossb->match_length[0] != 1) {
      ossb->match_length[match_count++] = 1;
    }
    /* 動態規劃求最短分割路徑 */
    for (j = 0; j < match_count; j++) {
      size_t k = ossb->match_length[j];
      ossb->match_length[j] = 0;
      if ((k > 1) && (ossb->min_len[i] + 1 <= ossb->min_len[i + k])) {
        ossb->min_len[i + k] = ossb->min_len[i] + 1;
        ossb->parent[i + k] = i;
      } else if ((k == 1) &&
                 (ossb->min_len[i] + 1 < ossb->min_len[i + k])) {
        ossb->min_len[i + k] = ossb->min_len[i] + 1;
        ossb->parent[i + k] = i;
      }
    }
  }
  /* 取得最短分割路徑 */
  for (i = length, j = ossb->min_len[length]; i != 0; i = ossb->parent[i]) {
    ossb->path[--j] = i;
  }
  size_t inbuf_left_start = *inbuf_left;
  size_t begin, end;
  /* 根據最短分割路徑轉換 */
  for (i = begin = 0; i < ossb->min_len[length]; i++) {
    end = ossb->path[i];
    size_t match_len;
    const ucs4_t* const* match_rs = dict_group_match_longest(
      converter->current_dict_group,
      *inbuf,
      end - begin,
      &match_len
      );
    if (match_rs == NULL) {
      **outbuf = **inbuf;
      (*outbuf)++, (*outbuf_left)--;
      (*inbuf)++, (*inbuf_left)--;
    } else {
      if (converter->conversion_mode == OPENCC_CONVERSION_FAST) {
        if (match_rs == NULL) {
          **outbuf = **inbuf;
          (*outbuf)++, (*outbuf_left)--;
          (*inbuf)++, (*inbuf_left)--;
        } else {
          const ucs4_t* result = match_rs[0];
          /* 輸出緩衝區剩餘空間小於分詞長度 */
          if (ucs4len(result) > *outbuf_left) {
            if (inbuf_left_start - *inbuf_left > 0) {
              break;
            }
            errnum = CONVERTER_ERROR_OUTBUF;
            return (size_t)-1;
          }
          for (; *result; result++) {
            **outbuf = *result;
            (*outbuf)++, (*outbuf_left)--;
          }
          *inbuf += match_len;
          *inbuf_left -= match_len;
        }
      } else if (converter->conversion_mode ==
                 OPENCC_CONVERSION_LIST_CANDIDATES) {
        if (match_rs == NULL) {
          **outbuf = **inbuf;
          (*outbuf)++, (*outbuf_left)--;
          (*inbuf)++, (*inbuf_left)--;
        } else {
          size_t i;
          for (i = 0; match_rs[i] != NULL; i++) {
            const ucs4_t* result = match_rs[i];
            int show_delimiter = match_rs[i + 1] != NULL ? 1 : 0;
            /* 輸出緩衝區剩餘空間小於分詞長度 */
            if (ucs4len(result) + show_delimiter > *outbuf_left) {
              if (inbuf_left_start - *inbuf_left > 0) {
                break;
              }
              errnum = CONVERTER_ERROR_OUTBUF;
              return (size_t)-1;
            }
            for (; *result; result++) {
              **outbuf = *result;
              (*outbuf)++, (*outbuf_left)--;
            }
            if (show_delimiter) {
              **outbuf = DELIMITER;
              (*outbuf)++, (*outbuf_left)--;
            }
          }
          *inbuf += match_len;
          *inbuf_left -= match_len;
        }
      } else if (converter->conversion_mode == OPENCC_CONVERSION_SEGMENT_ONLY) {
        if (match_rs == NULL) {
          **outbuf = **inbuf;
          (*outbuf)++, (*outbuf_left)--;
          (*inbuf)++, (*inbuf_left)--;
        } else {
          /* 輸出緩衝區剩餘空間小於分詞長度 */
          if (match_len + 1 > *outbuf_left) {
            if (inbuf_left_start - *inbuf_left > 0) {
              break;
            }
            errnum = CONVERTER_ERROR_OUTBUF;
            return (size_t)-1;
          }
          size_t i;
          for (i = 0; i < match_len; i++) {
            **outbuf = **inbuf;
            (*outbuf)++, (*outbuf_left)--;
            (*inbuf)++, (*inbuf_left)--;
          }
        }
        **outbuf = DELIMITER;
        (*outbuf)++, (*outbuf_left)--;
      } else {
        debug_should_not_be_here();
      }
    }
    begin = end;
  }
  return inbuf_left_start - *inbuf_left;
}

static size_t segment(Converter* converter,
                      ucs4_t** inbuf,
                      size_t* inbuf_left,
                      ucs4_t** outbuf,
                      size_t* outbuf_left) {
  /* 歧義分割最短路徑分詞 */
  size_t i, start, bound;
  const ucs4_t* inbuf_start = *inbuf;
  size_t inbuf_left_start = *inbuf_left;
  size_t sp_seg_length;
  bound = 0;
  for (i = start = 0; inbuf_start[i] && *inbuf_left > 0 && *outbuf_left > 0;
       i++) {
    if ((i != 0) && (i == bound)) {
      /* 對歧義部分進行最短路徑分詞 */
      sp_seg_length = sp_seg(converter,
                             inbuf,
                             inbuf_left,
                             outbuf,
                             outbuf_left,
                             bound - start);

      if (sp_seg_length ==  (size_t)-1) {
        return (size_t)-1;
      }
      if (sp_seg_length == 0) {
        if (inbuf_left_start - *inbuf_left > 0) {
          return inbuf_left_start - *inbuf_left;
        }
        /* 空間不足 */
        errnum = CONVERTER_ERROR_OUTBUF;
        return (size_t)-1;
      }
      start = i;
    }
    size_t match_len = 0;
    const ucs4_t* const* match_rs = dict_group_match_longest(
      converter->current_dict_group,
      inbuf_start + i,
      0,
      &match_len
      );
    if (match_rs == (const ucs4_t* const*)-1)
      return (size_t)-1;
    if (match_len == 0) {
      match_len = 1;
    }
    if (i + match_len > bound) {
      bound = i + match_len;
    }
  }
  if ((*inbuf_left > 0) && (*outbuf_left > 0)) {
    sp_seg_length = sp_seg(converter,
                           inbuf,
                           inbuf_left,
                           outbuf,
                           outbuf_left,
                           bound - start);
    if (sp_seg_length == (size_t)-1) {
      return (size_t)-1;
    }
    if (sp_seg_length == 0) {
      if (inbuf_left_start - *inbuf_left > 0) {
        return inbuf_left_start - *inbuf_left;
      }
      /* 空間不足 */
      errnum = CONVERTER_ERROR_OUTBUF;
      return (size_t)-1;
    }
  }
  if (converter->conversion_mode == OPENCC_CONVERSION_SEGMENT_ONLY) {
    (*outbuf)--;
    (*outbuf_left)++;
  }
  return inbuf_left_start - *inbuf_left;
}

#endif /* if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH */

#if SEGMENT_METHOD == SEGMENT_MAXIMUM_LENGTH
static size_t segment(Converter* converter,
                      ucs4_t** inbuf,
                      size_t* inbuf_left,
                      ucs4_t** outbuf,
                      size_t* outbuf_left) {
  /* 正向最大分詞 */
  size_t inbuf_left_start = *inbuf_left;
  for (; **inbuf && *inbuf_left > 0 && *outbuf_left > 0;) {
    size_t match_len;
    const ucs4_t* const* match_rs = dict_group_match_longest(
      converter->current_dict_group,
      *inbuf,
      *inbuf_left,
      &match_len
      );
    if (converter->conversion_mode == OPENCC_CONVERSION_FAST) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        const ucs4_t* result = match_rs[0];
        /* 輸出緩衝區剩餘空間小於分詞長度 */
        if (ucs4len(result) > *outbuf_left) {
          if (inbuf_left_start - *inbuf_left > 0) {
            break;
          }
          errnum = CONVERTER_ERROR_OUTBUF;
          return (size_t)-1;
        }
        for (; *result; result++) {
          **outbuf = *result;
          (*outbuf)++, (*outbuf_left)--;
        }
        *inbuf += match_len;
        *inbuf_left -= match_len;
      }
    } else if (converter->conversion_mode ==
               OPENCC_CONVERSION_LIST_CANDIDATES) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        size_t i;
        for (i = 0; match_rs[i] != NULL; i++) {
          const ucs4_t* result = match_rs[i];
          int show_delimiter = match_rs[i + 1] != NULL ? 1 : 0;
          /* 輸出緩衝區剩餘空間小於分詞長度 */
          if (ucs4len(result) + show_delimiter > *outbuf_left) {
            if (inbuf_left_start - *inbuf_left > 0) {
              break;
            }
            errnum = CONVERTER_ERROR_OUTBUF;
            return (size_t)-1;
          }
          for (; *result; result++) {
            **outbuf = *result;
            (*outbuf)++, (*outbuf_left)--;
          }
          if (show_delimiter) {
            **outbuf = DELIMITER;
            (*outbuf)++, (*outbuf_left)--;
          }
        }
        *inbuf += match_len;
        *inbuf_left -= match_len;
      }
    } else if (converter->conversion_mode == OPENCC_CONVERSION_SEGMENT_ONLY) {
      if (match_rs == NULL) {
        **outbuf = **inbuf;
        (*outbuf)++, (*outbuf_left)--;
        (*inbuf)++, (*inbuf_left)--;
      } else {
        /* 輸出緩衝區剩餘空間小於分詞長度 */
        if (match_len + 1 > *outbuf_left) {
          if (inbuf_left_start - *inbuf_left > 0) {
            break;
          }
          errnum = CONVERTER_ERROR_OUTBUF;
          return (size_t)-1;
        }
        size_t i;
        for (i = 0; i < match_len; i++) {
          **outbuf = **inbuf;
          (*outbuf)++, (*outbuf_left)--;
          (*inbuf)++, (*inbuf_left)--;
        }
      }
      **outbuf = DELIMITER;
      (*outbuf)++, (*outbuf_left)--;
    } else {
      debug_should_not_be_here();
    }
  }
  if (converter->conversion_mode == OPENCC_CONVERSION_SEGMENT_ONLY) {
    (*outbuf)--;
    (*outbuf_left)++;
  }
  return inbuf_left_start - *inbuf_left;
}

#endif /* if SEGMENT_METHOD == SEGMENT_MAXIMUM_LENGTH */

size_t converter_convert(Converter* converter,
                         ucs4_t** inbuf,
                         size_t* inbuf_left,
                         ucs4_t** outbuf,
                         size_t* outbuf_left) {
  if (converter->dict_chain == NULL) {
    errnum = CONVERTER_ERROR_NODICT;
    return (size_t)-1;
  }
  if (converter->dict_chain->count == 1) {
    /* 只有一個辭典，直接輸出 */
    return segment(converter,
							     inbuf,
							     inbuf_left,
							     outbuf,
							     outbuf_left);
  }
  // 啓用辭典轉換鏈
  size_t inbuf_size = *inbuf_left;
  size_t outbuf_size = *outbuf_left;
  size_t retval = (size_t)-1;
  size_t cinbuf_left, coutbuf_left;
  size_t coutbuf_delta = 0;
  size_t i, cur;
  ucs4_t* tmpbuf = (ucs4_t*)malloc(sizeof(ucs4_t) * outbuf_size);
  ucs4_t* orig_outbuf = *outbuf;
  ucs4_t* cinbuf, * coutbuf;
  cinbuf_left = inbuf_size;
  coutbuf_left = outbuf_size;
  cinbuf = *inbuf;
  coutbuf = tmpbuf;
  for (i = cur = 0; i < converter->dict_chain->count; ++i, cur = 1 - cur) {
    if (i > 0) {
      cinbuf_left = coutbuf_delta;
      coutbuf_left = outbuf_size;

      if (cur == 1) {
        cinbuf = tmpbuf;
        coutbuf = orig_outbuf;
      } else {
        cinbuf = orig_outbuf;
        coutbuf = tmpbuf;
      }
    }
    converter->current_dict_group = dict_chain_get_group(
      converter->dict_chain,
      i);
    size_t ret = segment(converter,
									      &cinbuf,
									      &cinbuf_left,
								        &coutbuf,
									      &coutbuf_left);
    if (ret == (size_t)-1) {
      free(tmpbuf);
      return (size_t)-1;
    }
    coutbuf_delta = outbuf_size - coutbuf_left;
    if (i == 0) {
      retval = ret;
      *inbuf = cinbuf;
      *inbuf_left = cinbuf_left;
    }
  }
  if (cur == 1) {
    // 結果在緩衝區
    memcpy(*outbuf, tmpbuf, coutbuf_delta * sizeof(ucs4_t));
  }
  *outbuf += coutbuf_delta;
  *outbuf_left = coutbuf_left;
  free(tmpbuf);
  return retval;
}

void converter_assign_dictionary(Converter* converter, DictChain* dict_chain) {
  converter->dict_chain = dict_chain;
  if (converter->dict_chain->count > 0) {
    converter->current_dict_group = dict_chain_get_group(
      converter->dict_chain,
      0);
  }
}

Converter* converter_open(void) {
  Converter* converter = (Converter*)malloc(sizeof(Converter));
  converter->dict_chain = NULL;
  converter->current_dict_group = NULL;
#if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH
  converter->data = (SpsegData*)malloc(sizeof(SpsegData));
  SpsegData* spseg_buffer = converter->data;
  spseg_buffer->initialized = 0;
  spseg_buffer->match_length = NULL;
  spseg_buffer->min_len = NULL;
  spseg_buffer->parent = NULL;
  spseg_buffer->path = NULL;
  sp_seg_set_buffer_size(spseg_buffer, OPENCC_SP_SEG_DEFAULT_BUFFER_SIZE);
#endif /* if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH */
  return converter;
}

void converter_close(Converter* converter) {
#if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH
  sp_seg_buffer_free(converter->data);
  free((SpsegData *)converter->data);
#endif /* if SEGMENT_METHOD == SEGMENT_SHORTEST_PATH */
  free(converter);
}

void converter_set_conversion_mode(Converter* converter,
                                   opencc_conversion_mode conversion_mode) {
  converter->conversion_mode = conversion_mode;
}

converter_error converter_errno(void) {
  return errnum;
}

void converter_perror(const char* spec) {
  perr(spec);
  perr("\n");
  switch (errnum) {
  case CONVERTER_ERROR_VOID:
    break;
  case CONVERTER_ERROR_NODICT:
    perr(_("No dictionary loaded"));
    break;
  case CONVERTER_ERROR_OUTBUF:
    perr(_("Output buffer not enough for one segment"));
    break;
  default:
    perr(_("Unknown"));
  }
}
