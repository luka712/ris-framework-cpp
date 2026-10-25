# Releasing

Version `0.1.0` lives in `CMakeLists.txt` (`project(... VERSION)`) and `vcpkg.json`. They must match.

1. Update `CHANGELOG.md`.
2. Push to the branch named after the version (`0.1.0`), let CI go green on Linux, Windows and macOS (shared and static).
3. Archives should contain `lib/`, `bin/` (Windows), `include/ris/`, and `share/ris_framework/` with `LICENSE`,
   `THIRD_PARTY_NOTICES.md` and a `third_party/` folder with the license texts of the linked libraries
   (collect them from `vcpkg_installed/<triplet>/share/*/copyright`).

A publish workflow (archives per platform, like ris-ktx2-cpp) is not set up yet.
