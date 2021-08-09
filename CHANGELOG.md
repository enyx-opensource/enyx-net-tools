# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

## [1.1.8] - 2021-08-09
### Changed
- The timeout timer will only start after all sessions have been configured
### Fixed
- Handling of `--cpu-cores` argument only considered the first range provided

## [1.1.7] - 2021-01-22
### Changed
- Decrease per session buffer size to 128K
- Rework threading model to use an `io_service` per thread

## [1.1.6] - 2020-10-19
## Removed
- Mixed std:cout output from multiple threads

## [1.1.5] - 2020-10-16
### Fixed
- Timeout computation
- Operation Canceled error with `--shutdown-policy = send_complete`

## [1.1.4] - 2020-08-03
### Fixed
- Bandwidth computation was wrong with a sampling != 1

## [1.1.3] - 2020-07-31
### Fixed
- Bandwidth computation was depending on the connection time

## [1.1.2] - 2020-07-30
### Fixed
- Typo on output 'bandwiDth' was missing one 'd'

## [1.1.1] - 2020-07-10
### Changed
- Tcp client listen & connect become asynchronous

## [1.1.0] - 2020-07-08
### Changed
- Session configurations can be read from stdin
### Added
- Sphinx documentation

## [1.0.0] - 2019-09-06
### Added
- Multiple sessions handling
### Changed
- Accept sessions parameters from response file
- Rename nx-iperf to `enyx-net-tester`

## [0.1.7] - 2019-04-25
### Fixed
- `-S` argument used for both `datagram-size` and `shutdown-policy`

## [0.1.6] - 2019-03-29
### Added
-  `--max-datagram-size` argument

## [0.1.5] - 2019-01-25
### Changed
- Allow UDP reception from any source

## [0.1.4] - 2019-01-21
### Fixed
- UDP bandwidth computing

## [0.1.3] - 2018-10-27
### Changed
- Return the POSIX error as process exit status.

## [0.1.2] - 2018-10-16
### Addded
- UDP Protocol
