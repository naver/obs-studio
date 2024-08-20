#!/usr/bin/env zsh

builtin emulate -L zsh
setopt EXTENDED_GLOB
setopt PUSHD_SILENT
setopt ERR_EXIT
setopt ERR_RETURN
setopt NO_UNSET
setopt PIPE_FAIL
setopt NO_AUTO_PUSHD
setopt NO_PUSHD_IGNORE_DUPS
setopt FUNCTION_ARGZERO

## Enable for script debugging
# setopt WARN_CREATE_GLOBAL
# setopt WARN_NESTED_VAR
# setopt XTRACE

autoload -Uz is-at-least && if ! is-at-least 5.2; then
  print -u2 -PR "%F{1}${funcstack[1]##*/}:%f Running on Zsh version %B${ZSH_VERSION}%b, but Zsh %B5.2%b is the minimum supported version. Upgrade zsh to fix this issue."
  exit 1
fi

invoke_formatter() {
  if (( # < 1 )) {
    log_error "Usage invoke_formatter [formatter_name]"
    exit 2
  }

  local formatter="${1}"
  shift
  local -a source_files=(${@})

  case ${formatter} {
    clang)
      if (( ${+commands[clang-format-17]} )) {
        local formatter=clang-format-17
      } elif (( ${+commands[clang-format]} )) {
        local formatter=clang-format
      } else {
        log_error "No viable clang-format version found (required 17.0.3)"
        exit 2
      }

      local -a formatter_version=($(${formatter} --version))

      if ! is-at-least 17.0.3 ${formatter_version[-1]}; then
        log_error "clang-format is not version 17.0.3 or above (found ${formatter_version[-1]}."
        exit 2
      fi

      if ! is-at-least ${formatter_version[-1]} 17.0.3; then
        log_error "clang-format is more recent than version 17.0.3 (found ${formatter_version[-1]})."
        exit 2
      fi

      if (( ! #source_files )) source_files=((libobs|libobs-*|UI|plugins|deps)/**/*.(c|cpp|h|hpp|m|mm)(.N))

      source_files=(${source_files:#*/(obs-websocket/deps|decklink/*/decklink-sdk|mac-syphon/syphon-framework|obs-outputs/ftl-sdk|win-dshow/libdshowcapture)/*})

      local -a format_args=(-style=file -fallback-style=none)
      if (( _loglevel > 2 )) format_args+=(--verbose)
      ;;
    cmake)
      local formatter=cmake-format
      if (( ${+commands[cmake-format]} )) {
        local cmake_format_version=$(cmake-format --version)

        if ! is-at-least 0.6.13 ${cmake_format_version}; then
          log_error "cmake-format is not version 0.6.13 or above (found ${cmake_format_version})."
          exit 2
        fi
      } else {
        log_error "No viable cmake-format version found (required 0.6.13)"
        exit 2
      }

      if (( ! #source_files )) source_files=((libobs|libobs-*|UI|plugins|deps|cmake)/**/(CMakeLists.txt|*.cmake)(.N))

      source_files=(${source_files:#*/(obs-outputs/ftl-sdk|jansson|decklink/*/decklink-sdk|obs-websocket|obs-browser|win-dshow/libdshowcapture)/*})

      local -a format_args=()
      if (( _loglevel > 2 )) format_args+=(--log-level debug)
      ;;
    swift)
      local formatter=swift-format
      if (( ${+commands[swift-format]} )) {
        local swift_format_version=$(swift-format --version)

        if ! is-at-least 508.0.0 ${swift_format_version}; then
          log_error "swift-format is not version 508.0.0 or above (found ${swift_format_version})."
          exit 2
        fi
      } else {
        log_error "No viable swift-format version found (required 508.0.0)"
        exit 2
      }

      if (( ! #source_files )) source_files=((libobs|libobs-*|UI|plugins)/**/*.swift(.N))

      local -a format_args=()
      ;;
    *) log_error "Invalid formatter specified: ${1}. Valid options are clang-format, cmake-format, and swift-format."; exit 2 ;;
  }

  local file
  local -i num_failures=0
  if (( check_only )) {
    for file (${source_files}) {
      if (( _loglevel > 1 )) log_info "Checking format of ${file}..."

      if ! "${formatter}" ${format_args} "${file}" | diff -q "${file}" - &> /dev/null; then
        log_error "${file} requires formatting changes."

        if (( fail_on_error == 2 )) return 2;
        num_failures=$(( num_failures + 1 ))
      else
        if (( _loglevel > 1 )) log_status "${file} requires no formatting changes."
      fi
    }
    if (( fail_on_error && num_failures )) return 2;
  } elif (( ${#source_files} )) {
    format_args+=(-i)
    "${formatter}" ${format_args} ${source_files}
  }
}

run_format() {
  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME=${ZSH_ARGZERO:A:h}
  if (( ! ${+FORMATTER_NAME} )) typeset -g FORMATTER_NAME=${${(s:-:)ZSH_ARGZERO:t:r}[2]}

  typeset -g host_os=${${(L)$(uname -s)}//darwin/macos}
  local -i fail_on_error=0
  local -i check_only=0
  local -i verbosity=1
  local -r _version='1.0.0'

  fpath=("${SCRIPT_HOME}/.functions" ${fpath})
  autoload -Uz set_loglevel log_info log_error log_output log_status log_warning

  local -r _usage="
Usage: %B${functrace[1]%:*}%b <option>

%BOptions%b:

%F{yellow} Formatting options%f
 -----------------------------------------------------------------------------
  %B-c | --check%b                      Check only, no actual formatting takes place

%F{yellow} Output options%f
 -----------------------------------------------------------------------------
  %B-v | --verbose%b                    Verbose (more detailed output)
  %B--fail-[never|error]                Fail script never/on formatting change - default: %B%F{green}never%f%b
  %B--debug%b                           Debug (very detailed and added output)

%F{yellow} General options%f
 -----------------------------------------------------------------------------
  %B-h | --help%b                       Print this usage help
  %B-V | --version%b                    Print script version information"

  local -a args
  while (( # )) {
    case ${1} {
      -c|--check) check_only=1; shift ;;
      -v|--verbose) (( verbosity += 1 )); shift ;;
      -h|--help) log_output ${_usage}; exit 0 ;;
      -V|--version) print -Pr "${_version}"; exit 0 ;;
      --debug) verbosity=3; shift ;;
      --fail-never)
        fail_on_error=0
        shift
        ;;
      --fail-error)
        fail_on_error=1
        shift
        ;;
      --fail-fast)
        fail_on_error=2
        shift
        ;;
      *)
        args+=($@)
        break
        ;;
    }
  }

  set -- ${(@)args}
  set_loglevel ${verbosity}

  invoke_formatter ${FORMATTER_NAME} ${args}
}

run_format ${@}
