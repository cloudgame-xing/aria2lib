#include "aria2_c_api.h"

#include "../aria2/src/includes/aria2/aria2.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

struct aria2_callback_ctx {
  aria2_download_event_callback callback;
  void* user_data;
  aria2_session_t* c_session;
};

struct aria2_session_t {
  aria2::Session* session;
  aria2_callback_ctx* callback_ctx;
};

struct aria2_download_handle_t {
  aria2::DownloadHandle* handle;
};

static char* aria2_strdup(const std::string& value)
{
  char* out = static_cast<char*>(std::malloc(value.size() + 1));
  if (!out) {
    return nullptr;
  }
  if (!value.empty()) {
    std::memcpy(out, value.data(), value.size());
  }
  out[value.size()] = '\0';
  return out;
}

static aria2_binary_t aria2_make_binary(const std::string& value)
{
  aria2_binary_t bin{};
  if (value.empty()) {
    return bin;
  }
  bin.data = static_cast<uint8_t*>(std::malloc(value.size()));
  if (!bin.data) {
    return bin;
  }
  std::memcpy(bin.data, value.data(), value.size());
  bin.length = value.size();
  return bin;
}

static aria2::KeyVals aria2_to_key_vals(const aria2_key_val_t* options,
                                        size_t options_count)
{
  aria2::KeyVals result;
  if (!options || options_count == 0) {
    return result;
  }
  result.reserve(options_count);
  for (size_t i = 0; i < options_count; ++i) {
    const char* key = options[i].key ? options[i].key : "";
    const char* value = options[i].value ? options[i].value : "";
    result.emplace_back(key, value);
  }
  return result;
}

static std::vector<std::string> aria2_to_string_vector(const char** values,
                                                       size_t count)
{
  std::vector<std::string> result;
  if (!values || count == 0) {
    return result;
  }
  result.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    result.emplace_back(values[i] ? values[i] : "");
  }
  return result;
}

static int aria2_copy_gid_vector(const std::vector<aria2::A2Gid>& gids,
                                 aria2_gid_t** out_gids,
                                 size_t* out_gids_count)
{
  if (!out_gids || !out_gids_count) {
    return -1;
  }
  *out_gids = nullptr;
  *out_gids_count = 0;
  if (gids.empty()) {
    return 0;
  }
  auto* data =
      static_cast<aria2_gid_t*>(std::malloc(sizeof(aria2_gid_t) * gids.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < gids.size(); ++i) {
    data[i] = static_cast<aria2_gid_t>(gids[i]);
  }
  *out_gids = data;
  *out_gids_count = gids.size();
  return 0;
}

static int aria2_copy_key_vals(const aria2::KeyVals& options,
                               aria2_key_val_t** out_options,
                               size_t* out_options_count)
{
  if (!out_options || !out_options_count) {
    return -1;
  }
  *out_options = nullptr;
  *out_options_count = 0;
  if (options.empty()) {
    return 0;
  }
  auto* data =
      static_cast<aria2_key_val_t*>(std::malloc(sizeof(aria2_key_val_t) * options.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < options.size(); ++i) {
    data[i].key = aria2_strdup(options[i].first);
    data[i].value = aria2_strdup(options[i].second);
    if ((options[i].first.size() && !data[i].key) ||
        (options[i].second.size() && !data[i].value)) {
      for (size_t j = 0; j <= i; ++j) {
        std::free(data[j].key);
        std::free(data[j].value);
      }
      std::free(data);
      return -1;
    }
  }
  *out_options = data;
  *out_options_count = options.size();
  return 0;
}

static int aria2_copy_uri_data(const std::vector<aria2::UriData>& uris,
                               aria2_uri_data_t** out_uris,
                               size_t* out_uris_count)
{
  if (!out_uris || !out_uris_count) {
    return -1;
  }
  *out_uris = nullptr;
  *out_uris_count = 0;
  if (uris.empty()) {
    return 0;
  }
  auto* data = static_cast<aria2_uri_data_t*>(
      std::malloc(sizeof(aria2_uri_data_t) * uris.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < uris.size(); ++i) {
    data[i].uri = aria2_strdup(uris[i].uri);
    data[i].status = static_cast<aria2_uri_status_t>(uris[i].status);
    if (uris[i].uri.size() && !data[i].uri) {
      for (size_t j = 0; j <= i; ++j) {
        std::free(data[j].uri);
      }
      std::free(data);
      return -1;
    }
  }
  *out_uris = data;
  *out_uris_count = uris.size();
  return 0;
}

static int aria2_copy_file_data(const aria2::FileData& file,
                                aria2_file_data_t* out_file)
{
  if (!out_file) {
    return -1;
  }
  *out_file = aria2_file_data_t{};
  out_file->index = file.index;
  out_file->path = aria2_strdup(file.path);
  out_file->length = file.length;
  out_file->completed_length = file.completedLength;
  out_file->selected = file.selected ? 1 : 0;
  if (file.path.size() && !out_file->path) {
    return -1;
  }
  if (aria2_copy_uri_data(file.uris, &out_file->uris,
                          &out_file->uris_count) != 0) {
    std::free(out_file->path);
    *out_file = aria2_file_data_t{};
    return -1;
  }
  return 0;
}

static int aria2_copy_file_data_vector(
    const std::vector<aria2::FileData>& files,
    aria2_file_data_t** out_files,
    size_t* out_files_count)
{
  if (!out_files || !out_files_count) {
    return -1;
  }
  *out_files = nullptr;
  *out_files_count = 0;
  if (files.empty()) {
    return 0;
  }
  auto* data = static_cast<aria2_file_data_t*>(
      std::malloc(sizeof(aria2_file_data_t) * files.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < files.size(); ++i) {
    if (aria2_copy_file_data(files[i], &data[i]) != 0) {
      for (size_t j = 0; j < i; ++j) {
        aria2_free_file_data(&data[j]);
      }
      std::free(data);
      return -1;
    }
  }
  *out_files = data;
  *out_files_count = files.size();
  return 0;
}

static int aria2_copy_string_list(const std::vector<std::string>& values,
                                  aria2_string_list_t* out_list)
{
  if (!out_list) {
    return -1;
  }
  out_list->values = nullptr;
  out_list->count = 0;
  if (values.empty()) {
    return 0;
  }
  auto* data =
      static_cast<char**>(std::malloc(sizeof(char*) * values.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < values.size(); ++i) {
    data[i] = aria2_strdup(values[i]);
    if (values[i].size() && !data[i]) {
      for (size_t j = 0; j <= i; ++j) {
        std::free(data[j]);
      }
      std::free(data);
      return -1;
    }
  }
  out_list->values = data;
  out_list->count = values.size();
  return 0;
}

static int aria2_copy_string_list_array(
    const std::vector<std::vector<std::string>>& lists,
    aria2_string_list_t** out_lists,
    size_t* out_lists_count)
{
  if (!out_lists || !out_lists_count) {
    return -1;
  }
  *out_lists = nullptr;
  *out_lists_count = 0;
  if (lists.empty()) {
    return 0;
  }
  auto* data = static_cast<aria2_string_list_t*>(
      std::malloc(sizeof(aria2_string_list_t) * lists.size()));
  if (!data) {
    return -1;
  }
  for (size_t i = 0; i < lists.size(); ++i) {
    if (aria2_copy_string_list(lists[i], &data[i]) != 0) {
      for (size_t j = 0; j < i; ++j) {
        aria2_free_string_list(&data[j]);
      }
      std::free(data);
      return -1;
    }
  }
  *out_lists = data;
  *out_lists_count = lists.size();
  return 0;
}

static int aria2_download_event_callback_proxy(aria2::Session* session,
                                               aria2::DownloadEvent event,
                                               aria2::A2Gid gid,
                                               void* userData)
{
  (void)session;
  auto* ctx = static_cast<aria2_callback_ctx*>(userData);
  if (!ctx || !ctx->callback) {
    return 0;
  }
  return ctx->callback(ctx->c_session,
                       static_cast<aria2_download_event_t>(event),
                       static_cast<aria2_gid_t>(gid),
                       ctx->user_data);
}

int aria2_library_init()
{
  return aria2::libraryInit();
}

int aria2_library_deinit()
{
  return aria2::libraryDeinit();
}

void aria2_session_config_init(aria2_session_config_t* config)
{
  if (!config) {
    return;
  }
  aria2::SessionConfig defaults;
  config->keep_running = defaults.keepRunning ? 1 : 0;
  config->use_signal_handler = defaults.useSignalHandler ? 1 : 0;
  config->download_event_callback = nullptr;
  config->user_data = nullptr;
}

aria2_session_t* aria2_session_new(const aria2_key_val_t* options,
                                   size_t options_count,
                                   const aria2_session_config_t* config)
{
  aria2::KeyVals cpp_options = aria2_to_key_vals(options, options_count);
  aria2::SessionConfig cpp_config;

  aria2_session_t* c_session =
      static_cast<aria2_session_t*>(std::malloc(sizeof(aria2_session_t)));
  if (!c_session) {
    return nullptr;
  }
  c_session->session = nullptr;
  c_session->callback_ctx = nullptr;

  if (config) {
    cpp_config.keepRunning = config->keep_running != 0;
    cpp_config.useSignalHandler = config->use_signal_handler != 0;
    if (config->download_event_callback) {
      aria2_callback_ctx* ctx =
          static_cast<aria2_callback_ctx*>(std::malloc(sizeof(*ctx)));
      if (!ctx) {
        std::free(c_session);
        return nullptr;
      }
      ctx->callback = config->download_event_callback;
      ctx->user_data = config->user_data;
      ctx->c_session = c_session;
      c_session->callback_ctx = ctx;
      cpp_config.downloadEventCallback = aria2_download_event_callback_proxy;
      cpp_config.userData = ctx;
    } else {
      cpp_config.userData = config->user_data;
    }
  }

  aria2::Session* session = aria2::sessionNew(cpp_options, cpp_config);
  if (!session) {
    std::free(c_session->callback_ctx);
    std::free(c_session);
    return nullptr;
  }
  c_session->session = session;
  return c_session;
}

int aria2_session_final(aria2_session_t* session)
{
  if (!session) {
    return 0;
  }
  int result = aria2::sessionFinal(session->session);
  std::free(session->callback_ctx);
  std::free(session);
  return result;
}

int aria2_run(aria2_session_t* session, aria2_run_mode_t mode)
{
  if (!session) {
    return -1;
  }
  return aria2::run(session->session, static_cast<aria2::RUN_MODE>(mode));
}

char* aria2_gid_to_hex(aria2_gid_t gid)
{
  return aria2_strdup(aria2::gidToHex(static_cast<aria2::A2Gid>(gid)));
}

aria2_gid_t aria2_hex_to_gid(const char* hex)
{
  if (!hex) {
    return static_cast<aria2_gid_t>(aria2::A2Gid());
  }
  return static_cast<aria2_gid_t>(aria2::hexToGid(hex));
}

int aria2_is_null(aria2_gid_t gid)
{
  return aria2::isNull(static_cast<aria2::A2Gid>(gid)) ? 1 : 0;
}

int aria2_add_uri(aria2_session_t* session,
                  aria2_gid_t* gid,
                        const char** uris,
                        size_t uris_count,
                        const aria2_key_val_t* options,
                        size_t options_count,
                        int position)
{
  if (!session) {
    return -1;
  }
  auto cpp_uris = aria2_to_string_vector(uris, uris_count);
  auto cpp_options = aria2_to_key_vals(options, options_count);
  aria2::A2Gid cpp_gid{};
  int result =
      aria2::addUri(session->session, gid ? &cpp_gid : nullptr, cpp_uris,
                    cpp_options, position);
  if (gid) {
    *gid = static_cast<aria2_gid_t>(cpp_gid);
  }
  return result;
}

int aria2_add_metalink(aria2_session_t* session,
                       aria2_gid_t** gids,
                             size_t* gids_count,
                             const char* metalink_file,
                             const aria2_key_val_t* options,
                             size_t options_count,
                             int position)
{
  if (!session) {
    return -1;
  }
  auto cpp_options = aria2_to_key_vals(options, options_count);
  std::vector<aria2::A2Gid> cpp_gids;
  std::vector<aria2::A2Gid>* cpp_gids_ptr = nullptr;
  if (gids && gids_count) {
    cpp_gids_ptr = &cpp_gids;
  }
  int result = aria2::addMetalink(
      session->session, cpp_gids_ptr,
      metalink_file ? metalink_file : "", cpp_options, position);
  if (result == 0 && cpp_gids_ptr) {
    if (aria2_copy_gid_vector(cpp_gids, gids, gids_count) != 0) {
      return -1;
    }
  }
  return result;
}

int aria2_add_torrent(aria2_session_t* session,
                      aria2_gid_t* gid,
                            const char* torrent_file,
                            const char** webseed_uris,
                            size_t webseed_uris_count,
                            const aria2_key_val_t* options,
                            size_t options_count,
                            int position)
{
  if (!session) {
    return -1;
  }
  auto cpp_webseed = aria2_to_string_vector(webseed_uris, webseed_uris_count);
  auto cpp_options = aria2_to_key_vals(options, options_count);
  aria2::A2Gid cpp_gid{};
  int result = aria2::addTorrent(
      session->session, gid ? &cpp_gid : nullptr,
      torrent_file ? torrent_file : "", cpp_webseed, cpp_options, position);
  if (gid) {
    *gid = static_cast<aria2_gid_t>(cpp_gid);
  }
  return result;
}

int aria2_add_torrent_simple(aria2_session_t* session,
                             aria2_gid_t* gid,
                                   const char* torrent_file,
                                   const aria2_key_val_t* options,
                                   size_t options_count,
                                   int position)
{
  if (!session) {
    return -1;
  }
  auto cpp_options = aria2_to_key_vals(options, options_count);
  aria2::A2Gid cpp_gid{};
  int result = aria2::addTorrent(session->session, gid ? &cpp_gid : nullptr,
                                 torrent_file ? torrent_file : "", cpp_options,
                                 position);
  if (gid) {
    *gid = static_cast<aria2_gid_t>(cpp_gid);
  }
  return result;
}

int aria2_get_active_download(aria2_session_t* session,
                              aria2_gid_t** gids,
                                    size_t* gids_count)
{
  if (!session) {
    return -1;
  }
  auto cpp_gids = aria2::getActiveDownload(session->session);
  return aria2_copy_gid_vector(cpp_gids, gids, gids_count);
}

int aria2_remove_download(aria2_session_t* session,
                          aria2_gid_t gid,
                                int force)
{
  if (!session) {
    return -1;
  }
  return aria2::removeDownload(session->session,
                               static_cast<aria2::A2Gid>(gid),
                               force != 0);
}

int aria2_pause_download(aria2_session_t* session,
                         aria2_gid_t gid,
                               int force)
{
  if (!session) {
    return -1;
  }
  return aria2::pauseDownload(session->session,
                              static_cast<aria2::A2Gid>(gid),
                              force != 0);
}

int aria2_unpause_download(aria2_session_t* session,
                           aria2_gid_t gid)
{
  if (!session) {
    return -1;
  }
  return aria2::unpauseDownload(session->session,
                                static_cast<aria2::A2Gid>(gid));
}

int aria2_change_option(aria2_session_t* session,
                        aria2_gid_t gid,
                        const aria2_key_val_t* options,
                              size_t options_count)
{
  if (!session) {
    return -1;
  }
  auto cpp_options = aria2_to_key_vals(options, options_count);
  return aria2::changeOption(session->session,
                             static_cast<aria2::A2Gid>(gid),
                             cpp_options);
}

char* aria2_get_global_option(aria2_session_t* session, const char* name)
{
  if (!session || !name) {
    return nullptr;
  }
  const auto& value = aria2::getGlobalOption(session->session, name);
  return aria2_strdup(value);
}

int aria2_get_global_options(aria2_session_t* session,
                             aria2_key_val_t** options,
                                   size_t* options_count)
{
  if (!session) {
    return -1;
  }
  auto cpp_options = aria2::getGlobalOptions(session->session);
  return aria2_copy_key_vals(cpp_options, options, options_count);
}

int aria2_change_global_option(aria2_session_t* session,
                               const aria2_key_val_t* options,
                                     size_t options_count)
{
  if (!session) {
    return -1;
  }
  auto cpp_options = aria2_to_key_vals(options, options_count);
  return aria2::changeGlobalOption(session->session, cpp_options);
}

aria2_global_stat_t aria2_get_global_stat(aria2_session_t* session)
{
  aria2_global_stat_t stat{};
  if (!session) {
    return stat;
  }
  auto cpp_stat = aria2::getGlobalStat(session->session);
  stat.download_speed = cpp_stat.downloadSpeed;
  stat.upload_speed = cpp_stat.uploadSpeed;
  stat.num_active = cpp_stat.numActive;
  stat.num_waiting = cpp_stat.numWaiting;
  stat.num_stopped = cpp_stat.numStopped;
  return stat;
}

int aria2_change_position(aria2_session_t* session,
                          aria2_gid_t gid,
                                int pos,
                                aria2_offset_mode_t how)
{
  if (!session) {
    return -1;
  }
  return aria2::changePosition(session->session,
                               static_cast<aria2::A2Gid>(gid),
                               pos,
                               static_cast<aria2::OffsetMode>(how));
}

int aria2_shutdown(aria2_session_t* session, int force)
{
  if (!session) {
    return -1;
  }
  return aria2::shutdown(session->session, force != 0);
}

aria2_download_handle_t* aria2_get_download_handle(aria2_session_t* session,
                                                   aria2_gid_t gid)
{
  if (!session) {
    return nullptr;
  }
  auto* handle =
      aria2::getDownloadHandle(session->session, static_cast<aria2::A2Gid>(gid));
  if (!handle) {
    return nullptr;
  }
  aria2_download_handle_t* c_handle =
      static_cast<aria2_download_handle_t*>(std::malloc(sizeof(*c_handle)));
  if (!c_handle) {
    aria2::deleteDownloadHandle(handle);
    return nullptr;
  }
  c_handle->handle = handle;
  return c_handle;
}

void aria2_delete_download_handle(aria2_download_handle_t* dh)
{
  if (!dh) {
    return;
  }
  aria2::deleteDownloadHandle(dh->handle);
  std::free(dh);
}

aria2_download_status_t
aria2_download_handle_get_status(aria2_download_handle_t* dh)
{
  if (!dh) {
    return ARIA2_DOWNLOAD_ERROR;
  }
  return static_cast<aria2_download_status_t>(dh->handle->getStatus());
}

int64_t aria2_download_handle_get_total_length(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getTotalLength() : 0;
}

int64_t aria2_download_handle_get_completed_length(
    aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getCompletedLength() : 0;
}

int64_t aria2_download_handle_get_upload_length(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getUploadLength() : 0;
}

aria2_binary_t aria2_download_handle_get_bitfield(
    aria2_download_handle_t* dh)
{
  if (!dh) {
    return aria2_binary_t{};
  }
  return aria2_make_binary(dh->handle->getBitfield());
}

int aria2_download_handle_get_download_speed(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getDownloadSpeed() : 0;
}

int aria2_download_handle_get_upload_speed(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getUploadSpeed() : 0;
}

aria2_binary_t aria2_download_handle_get_info_hash(
    aria2_download_handle_t* dh)
{
  if (!dh) {
    return aria2_binary_t{};
  }
  return aria2_make_binary(dh->handle->getInfoHash());
}

size_t aria2_download_handle_get_piece_length(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getPieceLength() : 0;
}

int aria2_download_handle_get_num_pieces(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getNumPieces() : 0;
}

int aria2_download_handle_get_connections(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getConnections() : 0;
}

int aria2_download_handle_get_error_code(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getErrorCode() : 0;
}

int aria2_download_handle_get_followed_by(aria2_download_handle_t* dh,
                                          aria2_gid_t** gids,
                                                size_t* gids_count)
{
  if (!dh) {
    return -1;
  }
  return aria2_copy_gid_vector(dh->handle->getFollowedBy(), gids,
                                     gids_count);
}

aria2_gid_t aria2_download_handle_get_following(
    aria2_download_handle_t* dh)
{
  return dh ? static_cast<aria2_gid_t>(dh->handle->getFollowing()) : 0;
}

aria2_gid_t aria2_download_handle_get_belongs_to(
    aria2_download_handle_t* dh)
{
  return dh ? static_cast<aria2_gid_t>(dh->handle->getBelongsTo()) : 0;
}

char* aria2_download_handle_get_dir(aria2_download_handle_t* dh)
{
  return dh ? aria2_strdup(dh->handle->getDir()) : nullptr;
}

int aria2_download_handle_get_files(aria2_download_handle_t* dh,
                                    aria2_file_data_t** files,
                                          size_t* files_count)
{
  if (!dh) {
    return -1;
  }
  return aria2_copy_file_data_vector(dh->handle->getFiles(), files,
                                           files_count);
}

int aria2_download_handle_get_num_files(aria2_download_handle_t* dh)
{
  return dh ? dh->handle->getNumFiles() : 0;
}

aria2_file_data_t aria2_download_handle_get_file(
    aria2_download_handle_t* dh,
    int index)
{
  aria2_file_data_t result{};
  if (!dh) {
    return result;
  }
  aria2::FileData file = dh->handle->getFile(index);
  if (aria2_copy_file_data(file, &result) != 0) {
    return aria2_file_data_t{};
  }
  return result;
}

aria2_bt_meta_info_data_t
aria2_download_handle_get_bt_meta_info(aria2_download_handle_t* dh)
{
  aria2_bt_meta_info_data_t result{};
  if (!dh) {
    return result;
  }
  aria2::BtMetaInfoData info = dh->handle->getBtMetaInfo();
  result.comment = aria2_strdup(info.comment);
  result.creation_date = static_cast<int64_t>(info.creationDate);
  result.mode = static_cast<aria2_bt_file_mode_t>(info.mode);
  result.name = aria2_strdup(info.name);
  if (aria2_copy_string_list_array(info.announceList,
                                   &result.announce_list,
                                   &result.announce_list_count) != 0) {
    aria2_free_bt_meta_info_data(&result);
    return aria2_bt_meta_info_data_t{};
  }
  return result;
}

char* aria2_download_handle_get_option(aria2_download_handle_t* dh,
                                       const char* name)
{
  if (!dh || !name) {
    return nullptr;
  }
  const auto& value = dh->handle->getOption(name);
  return aria2_strdup(value);
}

int aria2_download_handle_get_options(aria2_download_handle_t* dh,
                                      aria2_key_val_t** options,
                                            size_t* options_count)
{
  if (!dh) {
    return -1;
  }
  auto cpp_options = dh->handle->getOptions();
  return aria2_copy_key_vals(cpp_options, options, options_count);
}

void aria2_free(void* ptr)
{
  std::free(ptr);
}

void aria2_free_key_vals(aria2_key_val_t* options, size_t count)
{
  if (!options) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    std::free(options[i].key);
    std::free(options[i].value);
  }
  std::free(options);
}

void aria2_free_uri_data_array(aria2_uri_data_t* uris, size_t count)
{
  if (!uris) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    std::free(uris[i].uri);
  }
  std::free(uris);
}

void aria2_free_file_data(aria2_file_data_t* file)
{
  if (!file) {
    return;
  }
  std::free(file->path);
  if (file->uris) {
    aria2_free_uri_data_array(file->uris, file->uris_count);
  }
  file->path = nullptr;
  file->uris = nullptr;
  file->uris_count = 0;
}

void aria2_free_file_data_array(aria2_file_data_t* files, size_t count)
{
  if (!files) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    aria2_free_file_data(&files[i]);
  }
  std::free(files);
}

void aria2_free_string_list(aria2_string_list_t* list)
{
  if (!list) {
    return;
  }
  if (list->values) {
    for (size_t i = 0; i < list->count; ++i) {
      std::free(list->values[i]);
    }
    std::free(list->values);
  }
  list->values = nullptr;
  list->count = 0;
}

void aria2_free_string_list_array(aria2_string_list_t* lists, size_t count)
{
  if (!lists) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    aria2_free_string_list(&lists[i]);
  }
  std::free(lists);
}

void aria2_free_bt_meta_info_data(aria2_bt_meta_info_data_t* meta)
{
  if (!meta) {
    return;
  }
  if (meta->announce_list) {
    aria2_free_string_list_array(meta->announce_list,
                                 meta->announce_list_count);
  }
  std::free(meta->comment);
  std::free(meta->name);
  meta->announce_list = nullptr;
  meta->announce_list_count = 0;
  meta->comment = nullptr;
  meta->name = nullptr;
}

void aria2_free_binary(aria2_binary_t* bin)
{
  if (!bin) {
    return;
  }
  std::free(bin->data);
  bin->data = nullptr;
  bin->length = 0;
}
