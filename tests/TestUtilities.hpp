// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_433F11A724184ED38981B7B4E7B75D46
#define WJH_ATLAS_433F11A724184ED38981B7B4E7B75D46

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
    #include <unistd.h> // for mkdtemp (POSIX)
#endif

namespace wjh::atlas::testing {

/**
 * RAII wrapper for a temporary directory that auto-cleans on destruction.
 *
 * Creates a unique temporary directory and automatically removes it
 * (including all contents) when the object goes out of scope.
 *
 * Uses mkdtemp(3) on POSIX systems for atomic directory creation.
 *
 * Example usage:
 * @code
 * {
 *     TemporaryDirectory temp_dir;
 *     auto file_path = temp_dir.path() / "test.txt";
 *     std::ofstream(file_path) << "test content";
 *     // ... do testing ...
 * } // temp_dir and all contents automatically deleted here
 * @endcode
 */
class TemporaryDirectory
{
public:
    /**
     * Creates a unique temporary directory.
     *
     * @param prefix Optional prefix for the directory name
     * @throws std::runtime_error if directory creation fails
     */
    explicit TemporaryDirectory(
        std::filesystem::path const & prefix = "atlas_test_")
    {
#if defined(__unix__) || defined(__APPLE__)
        // POSIX: Use mkdtemp for atomic creation
        auto temp_base = std::filesystem::temp_directory_path();
        auto template_path = temp_base / (prefix.string() + "XXXXXX");
        auto template_str = template_path.string();

        // mkdtemp modifies the string in place
        std::vector<char> template_buf(
            template_str.begin(),
            template_str.end());
        template_buf.push_back('\0');

        if (::mkdtemp(template_buf.data()) == nullptr) {
            throw std::runtime_error(
                "mkdtemp failed: " + std::string(std::strerror(errno)));
        }

        path_ = std::filesystem::path(template_buf.data());
#else
        // Windows: Use random suffix with retry loop
        // Note: Still has a race condition window, but very small
        auto temp_base = std::filesystem::temp_directory_path();

        for (int attempts = 0; attempts < 100; ++attempts) {
            auto dir_name = prefix.string() + generate_random_suffix();
            path_ = temp_base / dir_name;

            std::error_code ec;
            if (std::filesystem::create_directory(path_, ec)) {
                return; // Success
            }

            if (ec && ec != std::errc::file_exists) {
                throw std::runtime_error(
                    "Failed to create temporary directory: " + ec.message());
            }
            // If file_exists, try again with new random suffix
        }

        throw std::runtime_error(
            "Failed to create unique temporary directory after 100 attempts");
#endif
    }

    // Non-copyable
    TemporaryDirectory(TemporaryDirectory const &) = delete;
    TemporaryDirectory & operator = (TemporaryDirectory const &) = delete;

    // Movable
    TemporaryDirectory(TemporaryDirectory && other) noexcept
    : path_(std::move(other.path_))
    {
        other.path_.clear(); // Prevent other from cleaning up
    }

    TemporaryDirectory & operator = (TemporaryDirectory && other) noexcept
    {
        if (this != &other) {
            cleanup();
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    /**
     * Destroys the temporary directory and all its contents.
     */
    ~TemporaryDirectory() { cleanup(); }

    /**
     * Returns the path to the temporary directory.
     */
    [[nodiscard]]
    std::filesystem::path const & path() const noexcept
    {
        return path_;
    }

    /**
     * Implicit conversion to filesystem::path for convenience.
     */
    operator std::filesystem::path const & () const noexcept { return path_; }

private:
    std::filesystem::path path_;

    void cleanup() noexcept
    {
        if (not path_.empty() && std::filesystem::exists(path_)) {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
            // Ignore errors during cleanup (best effort)
        }
    }

    static std::string generate_random_suffix()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::ostringstream oss;
        for (int i = 0; i < 16; ++i) {
            oss << std::hex << dis(gen);
        }
        return oss.str();
    }
};

/**
 * Helper to write a string to a file.
 *
 * @param path File path to write to
 * @param content Content to write
 * @throws std::runtime_error if writing fails
 */
inline void
write_file(std::filesystem::path const & path, std::string const & content)
{
    std::ofstream file(path);
    if (not file) {
        throw std::runtime_error(
            "Cannot open file for writing: " + path.string());
    }
    file << content;
    if (not file) {
        throw std::runtime_error("Error writing to file: " + path.string());
    }
}

/**
 * Helper to read a file to a string.
 *
 * @param path File path to read from
 * @return File contents as string
 * @throws std::runtime_error if reading fails
 */
inline std::string
read_file(std::filesystem::path const & path)
{
    std::ifstream file(path);
    if (not file) {
        throw std::runtime_error(
            "Cannot open file for reading: " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    if (not file && not file.eof()) {
        throw std::runtime_error("Error reading file: " + path.string());
    }
    return ss.str();
}

} // namespace wjh::atlas::testing

#endif // WJH_ATLAS_433F11A724184ED38981B7B4E7B75D46
