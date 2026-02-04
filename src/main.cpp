#include <chrono>
#include <cstdlib>
#include <iostream>

#include "aria2_c_api.h"

int download_event_callback(aria2_session_t* session,
                            aria2_download_event_t event,
                            aria2_gid_t gid,
                            void* user_data)
{
  (void)user_data;
  switch (event) {
  case ARIA2_EVENT_ON_DOWNLOAD_COMPLETE:
    std::cerr << "COMPLETE";
    break;
  case ARIA2_EVENT_ON_DOWNLOAD_ERROR:
    std::cerr << "ERROR";
    break;
  default:
    return 0;
  }

  char* gid_hex = aria2_gid_to_hex(gid);
  if (gid_hex) {
    std::cerr << " [" << gid_hex << "] ";
    aria2_free(gid_hex);
  }
  else {
    std::cerr << " [unknown] ";
  }

  aria2_download_handle_t* dh = aria2_get_download_handle(session, gid);
  if (!dh) {
    std::cerr << std::endl;
    return 0;
  }

  if (aria2_download_handle_get_num_files(dh) > 0) {
    aria2_file_data_t f = aria2_download_handle_get_file(dh, 1);
    if (f.path && f.path[0] != '\0') {
      std::cerr << f.path;
    }
    else if (f.uris_count > 0 && f.uris && f.uris[0].uri) {
      std::cerr << f.uris[0].uri;
    }
    aria2_free_file_data(&f);
  }

  aria2_delete_download_handle(dh);
  std::cerr << std::endl;
  return 0;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    std::cerr << "Usage: aria2_c_api_main URI [URI...]\n\n"
              << "  Download given URIs in parallel in the current directory."
              << std::endl;
    return 0;
  }

  int rv = aria2_library_init();
  if (rv != 0) {
    std::cerr << "aria2 init failed: " << rv << std::endl;
    return 1;
  }

  aria2_session_config_t config{};
  aria2_session_config_init(&config);
  config.download_event_callback = download_event_callback;

  aria2_session_t* session = aria2_session_new(nullptr, 0, &config);
  if (!session) {
    std::cerr << "aria2 session create failed" << std::endl;
    aria2_library_deinit();
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    const char* uris[] = {argv[i]};
    rv = aria2_add_uri(session, nullptr, uris, 1, nullptr, 0, -1);
    if (rv < 0) {
      std::cerr << "Failed to add download " << argv[i] << std::endl;
    }
  }

  auto start = std::chrono::steady_clock::now();
  for (;;) {
    rv = aria2_run(session, ARIA2_RUN_ONCE);
    if (rv != 1) {
      break;
    }
    auto now = std::chrono::steady_clock::now();
    auto count =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();
    if (count >= 500) {
      start = now;
      aria2_global_stat_t gstat = aria2_get_global_stat(session);
      std::cerr << "Overall #Active:" << gstat.num_active
                << " #waiting:" << gstat.num_waiting
                << " D:" << gstat.download_speed / 1024 << "KiB/s"
                << " U:" << gstat.upload_speed / 1024 << "KiB/s "
                << std::endl;

      aria2_gid_t* gids = nullptr;
      size_t gids_count = 0;
      if (aria2_get_active_download(session, &gids, &gids_count) == 0) {
        for (size_t i = 0; i < gids_count; ++i) {
          aria2_download_handle_t* dh =
              aria2_get_download_handle(session, gids[i]);
          if (dh) {
            int64_t completed =
                aria2_download_handle_get_completed_length(dh);
            int64_t total = aria2_download_handle_get_total_length(dh);
            int progress =
                total > 0 ? static_cast<int>(100 * completed / total) : 0;
            std::cerr << "    [";
            char* gid_hex = aria2_gid_to_hex(gids[i]);
            if (gid_hex) {
              std::cerr << gid_hex;
              aria2_free(gid_hex);
            }
            else {
              std::cerr << "unknown";
            }
            std::cerr << "] " << completed << "/" << total << "(" << progress
                      << "%)"
                      << " D:"
                      << aria2_download_handle_get_download_speed(dh) / 1024
                      << "KiB/s, U:"
                      << aria2_download_handle_get_upload_speed(dh) / 1024
                      << "KiB/s" << std::endl;
            aria2_delete_download_handle(dh);
          }
        }
      }
      aria2_free(gids);
    }
  }

  rv = aria2_session_final(session);
  aria2_library_deinit();
  return rv;
}
