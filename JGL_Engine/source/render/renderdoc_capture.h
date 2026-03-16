#pragma once

#include <string>

namespace nrender
{
  class RenderDocCapture
  {
  public:
    struct Api;

    RenderDocCapture();
    ~RenderDocCapture();

    RenderDocCapture(const RenderDocCapture&) = delete;
    RenderDocCapture& operator=(const RenderDocCapture&) = delete;

    bool is_available() const;
    bool is_capture_pending() const;
    bool is_capture_in_progress() const;
    const std::string& last_capture_path() const;

    void request_capture();
    bool begin_capture(void* native_window, const std::string& capture_title);
    void end_capture(void* native_window);

  private:
    void initialize();

  private:
    void* mModule = nullptr;
    bool mOwnsModule = false;
    Api* mApi = nullptr;
    bool mCapturePending = false;
    bool mCaptureInProgress = false;
    std::string mLastCapturePath;
  };
}
