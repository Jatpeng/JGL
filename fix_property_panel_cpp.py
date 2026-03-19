import re
with open("JGL_Engine/source/ui/property_panel.cpp", "r") as f:
    text = f.read()

# Replace block 1
search1 = """<<<<<<< HEAD
      std::wstring result(size, 0);
      MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, result.data(), size);
      return result;
=======
      if (size <= 1)
        return {};

      std::vector<wchar_t> buffer(static_cast<size_t>(size), L'\\0');
      MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, buffer.data(), size);
      return std::wstring(buffer.data());
#else
      return std::wstring(narrow.begin(), narrow.end());
#endif
>>>>>>> origin/master"""

replace1 = """      if (size <= 1)
        return {};

      std::vector<wchar_t> buffer(static_cast<size_t>(size), L'\\0');
      MultiByteToWideChar(CP_ACP, 0, narrow.c_str(), -1, buffer.data(), size);
      return std::wstring(buffer.data());
#else
      return std::wstring(narrow.begin(), narrow.end());
#endif"""

text = text.replace(search1, replace1)

# Replace block 2
search2 = """<<<<<<< HEAD
      std::string result(size, 0);
      WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, result.data(), size, nullptr, nullptr);
      return result;
=======
      if (size <= 1)
        return {};

      std::vector<char> buffer(static_cast<size_t>(size), '\\0');
      WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, buffer.data(), size, nullptr, nullptr);
      return std::string(buffer.data());
#else
      return std::string(wide.begin(), wide.end());
#endif
>>>>>>> origin/master"""

replace2 = """      if (size <= 1)
        return {};

      std::vector<char> buffer(static_cast<size_t>(size), '\\0');
      WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, buffer.data(), size, nullptr, nullptr);
      return std::string(buffer.data());
#else
      return std::string(wide.begin(), wide.end());
#endif"""

text = text.replace(search2, replace2)

# Replace block 3
search3 = """<<<<<<< HEAD
      OPENFILENAMEW dialog;
      ZeroMemory(&dialog, sizeof(dialog));
=======
#ifdef _WIN32
      std::array<wchar_t, 4096> filename = {};
      const std::wstring initial_dir = wide_from_narrow(FileSystem::getPath(""));
      OPENFILENAMEW dialog = {};
      dialog.lStructSize = sizeof(dialog);
>>>>>>> origin/master
      dialog.hwndOwner = GetActiveWindow();
      dialog.lStructSize = sizeof(dialog);
      std::array<wchar_t, MAX_PATH> filename = { 0 };
      dialog.lpstrFile = filename.data();
      dialog.nMaxFile = MAX_PATH;
      dialog.lpstrFilter = filter;
      dialog.lpstrTitle = title;
      dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

      if (GetOpenFileNameW(&dialog) == 0)
        return {};

      return narrow_from_wide(filename.data());
    }
#else
    std::optional<std::string> open_native_file_dialog(const wchar_t* title, const wchar_t* filter)
    {
      return {};
    }
#endif"""

replace3 = """#ifdef _WIN32
      std::array<wchar_t, 4096> filename = {};
      const std::wstring initial_dir = wide_from_narrow(FileSystem::getPath(""));
      OPENFILENAMEW dialog = {};
      dialog.lStructSize = sizeof(dialog);
      dialog.hwndOwner = GetActiveWindow();
      dialog.lpstrFile = filename.data();
      dialog.nMaxFile = static_cast<DWORD>(filename.size());
      dialog.lpstrInitialDir = initial_dir.empty() ? nullptr : initial_dir.c_str();
      dialog.lpstrFilter = filter;
      dialog.nFilterIndex = 1;
      dialog.lpstrTitle = title;
      dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

      if (!GetOpenFileNameW(&dialog))
        return std::nullopt;

      return narrow_from_wide(filename.data());
    }
#else
    std::optional<std::string> open_native_file_dialog(const wchar_t* title, const wchar_t* filter)
    {
      return std::nullopt;
    }
#endif"""

text = text.replace(search3, replace3)

with open("JGL_Engine/source/ui/property_panel.cpp", "w") as f:
    f.write(text)
