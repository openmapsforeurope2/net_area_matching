## [1.1.0] - 2026-09-17

### Added

* Added support for inland water areas in the matching process.
* Added support for processing areas connecting more than two countries.
* Added cutting points to improve the processing of network areas.
* Added the `PC_COUNTRY_BUFFER` parameter to configure the country buffer used during processing.
* Added a command-line parameter to specify the target database name.

### Changed

* Changed the output table suffix so that it is based only on the user-defined suffix and no longer includes country codes.
* Updated the handling of destroyed objects throughout the processing.
* Updated the parameter names and configuration to improve consistency.
* Updated the SOCLE, external libraries and EPG dependencies.
* Adapted the application to the IGN-MUT deployment environment.
* Improved cleanup of artefacts.

### Fixed

* Fixed the initialization of the working area in `PolygonCleanerOp`.
* Fixed memory leaks identified through Valgrind analysis.
* Improved error handling and diagnostic information.
* Fixed database table naming and IGN-MUT environment variable handling.

## [1.0.0] - 2025-03-24
### Added
- Initial release of the project

### Changed
- NTR

### Fixed
- NTR