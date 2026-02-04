#include <iostream>

#include "aria2_c_api.h"

int main()
{
  const int init_result = aria2_c_api_library_init();
  std::cout << "aria2 init result: " << init_result << std::endl;
  if (init_result != 0) {
    return 1;
  }

  aria2_c_api_session_config_t config{};
  aria2_c_api_session_config_init(&config);

  aria2_session_t* session = aria2_c_api_session_new(nullptr, 0, &config);
  if (!session) {
    std::cout << "aria2 session create failed" << std::endl;
    aria2_c_api_library_deinit();
    return 1;
  }

  aria2_c_api_global_stat_t stat = aria2_c_api_get_global_stat(session);
  std::cout << "global download speed: " << stat.download_speed << std::endl;
  std::cout << "global upload speed: " << stat.upload_speed << std::endl;

  aria2_c_api_key_val_t* options = nullptr;
  size_t options_count = 0;
  if (aria2_c_api_get_global_options(session, &options, &options_count) == 0) {
    std::cout << "global options count: " << options_count << std::endl;
    if (options_count > 0 && options[0].key && options[0].value) {
      std::cout << "first option: " << options[0].key << "=" << options[0].value
                << std::endl;
    }
  }
  aria2_c_api_free_key_vals(options, options_count);

  const int final_result = aria2_c_api_session_final(session);
  std::cout << "aria2 session final result: " << final_result << std::endl;
  aria2_c_api_library_deinit();
  return 0;
}
