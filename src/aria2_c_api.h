#ifndef ARIA2_C_API_H
#define ARIA2_C_API_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  if defined(ARIA2_C_API_BUILD)
#    define ARIA2_C_API __declspec(dllexport)
#  else
#    define ARIA2_C_API __declspec(dllimport)
#  endif
#else
#  define ARIA2_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aria2_session_t aria2_session_t;
typedef struct aria2_download_handle_t aria2_download_handle_t;

typedef uint64_t aria2_c_api_gid_t;

typedef enum {
  ARIA2_C_API_EVENT_ON_DOWNLOAD_START = 1,
  ARIA2_C_API_EVENT_ON_DOWNLOAD_PAUSE,
  ARIA2_C_API_EVENT_ON_DOWNLOAD_STOP,
  ARIA2_C_API_EVENT_ON_DOWNLOAD_COMPLETE,
  ARIA2_C_API_EVENT_ON_DOWNLOAD_ERROR,
  ARIA2_C_API_EVENT_ON_BT_DOWNLOAD_COMPLETE
} aria2_c_api_download_event_t;

typedef enum {
  ARIA2_C_API_RUN_DEFAULT,
  ARIA2_C_API_RUN_ONCE
} aria2_c_api_run_mode_t;

typedef enum {
  ARIA2_C_API_OFFSET_MODE_SET,
  ARIA2_C_API_OFFSET_MODE_CUR,
  ARIA2_C_API_OFFSET_MODE_END
} aria2_c_api_offset_mode_t;

typedef enum {
  ARIA2_C_API_URI_USED,
  ARIA2_C_API_URI_WAITING
} aria2_c_api_uri_status_t;

typedef enum {
  ARIA2_C_API_BT_FILE_MODE_NONE,
  ARIA2_C_API_BT_FILE_MODE_SINGLE,
  ARIA2_C_API_BT_FILE_MODE_MULTI
} aria2_c_api_bt_file_mode_t;

typedef enum {
  ARIA2_C_API_DOWNLOAD_ACTIVE,
  ARIA2_C_API_DOWNLOAD_WAITING,
  ARIA2_C_API_DOWNLOAD_PAUSED,
  ARIA2_C_API_DOWNLOAD_COMPLETE,
  ARIA2_C_API_DOWNLOAD_ERROR,
  ARIA2_C_API_DOWNLOAD_REMOVED
} aria2_c_api_download_status_t;

typedef int (*aria2_c_api_download_event_callback)(aria2_session_t* session,
                                                   aria2_c_api_download_event_t event,
                                                   aria2_c_api_gid_t gid,
                                                   void* user_data);

typedef struct {
  int keep_running;
  int use_signal_handler;
  aria2_c_api_download_event_callback download_event_callback;
  void* user_data;
} aria2_c_api_session_config_t;

typedef struct {
  char* key;
  char* value;
} aria2_c_api_key_val_t;

typedef struct {
  int download_speed;
  int upload_speed;
  int num_active;
  int num_waiting;
  int num_stopped;
} aria2_c_api_global_stat_t;

typedef struct {
  char* uri;
  aria2_c_api_uri_status_t status;
} aria2_c_api_uri_data_t;

typedef struct {
  int index;
  char* path;
  int64_t length;
  int64_t completed_length;
  int selected;
  aria2_c_api_uri_data_t* uris;
  size_t uris_count;
} aria2_c_api_file_data_t;

typedef struct {
  char** values;
  size_t count;
} aria2_c_api_string_list_t;

typedef struct {
  aria2_c_api_string_list_t* announce_list;
  size_t announce_list_count;
  char* comment;
  int64_t creation_date;
  aria2_c_api_bt_file_mode_t mode;
  char* name;
} aria2_c_api_bt_meta_info_data_t;

typedef struct {
  uint8_t* data;
  size_t length;
} aria2_c_api_binary_t;

ARIA2_C_API int aria2_c_api_library_init();
ARIA2_C_API int aria2_c_api_library_deinit();

ARIA2_C_API void aria2_c_api_session_config_init(aria2_c_api_session_config_t* config);

ARIA2_C_API aria2_session_t* aria2_c_api_session_new(
    const aria2_c_api_key_val_t* options,
    size_t options_count,
    const aria2_c_api_session_config_t* config);
ARIA2_C_API int aria2_c_api_session_final(aria2_session_t* session);

ARIA2_C_API int aria2_c_api_run(aria2_session_t* session, aria2_c_api_run_mode_t mode);

ARIA2_C_API char* aria2_c_api_gid_to_hex(aria2_c_api_gid_t gid);
ARIA2_C_API aria2_c_api_gid_t aria2_c_api_hex_to_gid(const char* hex);
ARIA2_C_API int aria2_c_api_is_null(aria2_c_api_gid_t gid);

ARIA2_C_API int aria2_c_api_add_uri(aria2_session_t* session,
                                   aria2_c_api_gid_t* gid,
                                   const char** uris,
                                   size_t uris_count,
                                   const aria2_c_api_key_val_t* options,
                                   size_t options_count,
                                   int position);

ARIA2_C_API int aria2_c_api_add_metalink(aria2_session_t* session,
                                        aria2_c_api_gid_t** gids,
                                        size_t* gids_count,
                                        const char* metalink_file,
                                        const aria2_c_api_key_val_t* options,
                                        size_t options_count,
                                        int position);

ARIA2_C_API int aria2_c_api_add_torrent(aria2_session_t* session,
                                       aria2_c_api_gid_t* gid,
                                       const char* torrent_file,
                                       const char** webseed_uris,
                                       size_t webseed_uris_count,
                                       const aria2_c_api_key_val_t* options,
                                       size_t options_count,
                                       int position);

ARIA2_C_API int aria2_c_api_add_torrent_simple(aria2_session_t* session,
                                              aria2_c_api_gid_t* gid,
                                              const char* torrent_file,
                                              const aria2_c_api_key_val_t* options,
                                              size_t options_count,
                                              int position);

ARIA2_C_API int aria2_c_api_get_active_download(aria2_session_t* session,
                                               aria2_c_api_gid_t** gids,
                                               size_t* gids_count);

ARIA2_C_API int aria2_c_api_remove_download(aria2_session_t* session,
                                           aria2_c_api_gid_t gid,
                                           int force);
ARIA2_C_API int aria2_c_api_pause_download(aria2_session_t* session,
                                          aria2_c_api_gid_t gid,
                                          int force);
ARIA2_C_API int aria2_c_api_unpause_download(aria2_session_t* session,
                                            aria2_c_api_gid_t gid);

ARIA2_C_API int aria2_c_api_change_option(aria2_session_t* session,
                                         aria2_c_api_gid_t gid,
                                         const aria2_c_api_key_val_t* options,
                                         size_t options_count);

ARIA2_C_API char* aria2_c_api_get_global_option(aria2_session_t* session,
                                               const char* name);
ARIA2_C_API int aria2_c_api_get_global_options(aria2_session_t* session,
                                              aria2_c_api_key_val_t** options,
                                              size_t* options_count);
ARIA2_C_API int aria2_c_api_change_global_option(aria2_session_t* session,
                                                const aria2_c_api_key_val_t* options,
                                                size_t options_count);

ARIA2_C_API aria2_c_api_global_stat_t aria2_c_api_get_global_stat(
    aria2_session_t* session);

ARIA2_C_API int aria2_c_api_change_position(aria2_session_t* session,
                                           aria2_c_api_gid_t gid,
                                           int pos,
                                           aria2_c_api_offset_mode_t how);

ARIA2_C_API int aria2_c_api_shutdown(aria2_session_t* session, int force);

ARIA2_C_API aria2_download_handle_t* aria2_c_api_get_download_handle(
    aria2_session_t* session,
    aria2_c_api_gid_t gid);
ARIA2_C_API void aria2_c_api_delete_download_handle(aria2_download_handle_t* dh);

ARIA2_C_API aria2_c_api_download_status_t
aria2_c_api_download_handle_get_status(aria2_download_handle_t* dh);
ARIA2_C_API int64_t aria2_c_api_download_handle_get_total_length(
    aria2_download_handle_t* dh);
ARIA2_C_API int64_t aria2_c_api_download_handle_get_completed_length(
    aria2_download_handle_t* dh);
ARIA2_C_API int64_t aria2_c_api_download_handle_get_upload_length(
    aria2_download_handle_t* dh);
ARIA2_C_API aria2_c_api_binary_t aria2_c_api_download_handle_get_bitfield(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_download_speed(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_upload_speed(
    aria2_download_handle_t* dh);
ARIA2_C_API aria2_c_api_binary_t aria2_c_api_download_handle_get_info_hash(
    aria2_download_handle_t* dh);
ARIA2_C_API size_t aria2_c_api_download_handle_get_piece_length(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_num_pieces(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_connections(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_error_code(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_followed_by(
    aria2_download_handle_t* dh,
    aria2_c_api_gid_t** gids,
    size_t* gids_count);
ARIA2_C_API aria2_c_api_gid_t aria2_c_api_download_handle_get_following(
    aria2_download_handle_t* dh);
ARIA2_C_API aria2_c_api_gid_t aria2_c_api_download_handle_get_belongs_to(
    aria2_download_handle_t* dh);
ARIA2_C_API char* aria2_c_api_download_handle_get_dir(
    aria2_download_handle_t* dh);
ARIA2_C_API int aria2_c_api_download_handle_get_files(
    aria2_download_handle_t* dh,
    aria2_c_api_file_data_t** files,
    size_t* files_count);
ARIA2_C_API int aria2_c_api_download_handle_get_num_files(
    aria2_download_handle_t* dh);
ARIA2_C_API aria2_c_api_file_data_t aria2_c_api_download_handle_get_file(
    aria2_download_handle_t* dh,
    int index);
ARIA2_C_API aria2_c_api_bt_meta_info_data_t
aria2_c_api_download_handle_get_bt_meta_info(aria2_download_handle_t* dh);
ARIA2_C_API char* aria2_c_api_download_handle_get_option(
    aria2_download_handle_t* dh,
    const char* name);
ARIA2_C_API int aria2_c_api_download_handle_get_options(
    aria2_download_handle_t* dh,
    aria2_c_api_key_val_t** options,
    size_t* options_count);

/*
 * 释放由本 C API 分配的内存。所有返回的字符串、数组、
 * 以及包含深层数据的结构体都应使用下面的函数释放。
 */
ARIA2_C_API void aria2_c_api_free(void* ptr);
ARIA2_C_API void aria2_c_api_free_key_vals(aria2_c_api_key_val_t* options,
                                          size_t count);
ARIA2_C_API void aria2_c_api_free_uri_data_array(aria2_c_api_uri_data_t* uris,
                                                size_t count);
ARIA2_C_API void aria2_c_api_free_file_data(aria2_c_api_file_data_t* file);
ARIA2_C_API void aria2_c_api_free_file_data_array(aria2_c_api_file_data_t* files,
                                                 size_t count);
ARIA2_C_API void aria2_c_api_free_string_list(aria2_c_api_string_list_t* list);
ARIA2_C_API void aria2_c_api_free_string_list_array(
    aria2_c_api_string_list_t* lists,
    size_t count);
ARIA2_C_API void aria2_c_api_free_bt_meta_info_data(
    aria2_c_api_bt_meta_info_data_t* meta);
ARIA2_C_API void aria2_c_api_free_binary(aria2_c_api_binary_t* bin);

#ifdef __cplusplus
}
#endif

#endif
