#!/bin/sh
set -e

if ! test -d "src"; then
    echo "Please run this in the project root directory."
    exit 1
fi

# Note: needs faust >= 2.27.1 for UI macros
FAUSTARGS="-uim -inpl"

# support GNU sed only, use gsed on a Mac
test -z "$SED" && SED=sed

faustgen() {
    mkdir -p src/sfizz/effects/gen
    local outfile=src/sfizz/effects/gen/compressor.cxx

    local code=`faust $FAUSTARGS -cn faustCompressor src/sfizz/effects/dsp/compressor.dsp`

    # suppress some faust-specific stuff we don't care
    echo "$code" \
          | fgrep -v -- '->declare(' \
          | fgrep -v -- '->openHorizontalBox(' \
          | fgrep -v -- '->openVerticalBox(' \
          | fgrep -v -- '->closeBox(' \
          | fgrep -v -- '->addHorizontalSlider(' \
          | fgrep -v -- '->addVerticalSlider(' \
          > "$outfile"

    # remove metadata
    $SED -r -i 's/void[ \t]+metadata[ \t]*\(Meta[ \t]*\*[ \t]*[a-zA-Z0-9_]+\)/void metadata()/' "$outfile"

    # remove UI
    $SED -r -i 's/void[ \t]+buildUserInterface[ \t]*\(UI[ \t]*\*[ \t]*[a-zA-Z0-9_]+\)/void buildUserInterface()/' "$outfile"

    # remove inheritance
    $SED -r -i 's/:[ \t]*public[ \t]+dsp\b\s*//' "$outfile"

    # remove virtual
    $SED -r -i 's/\bvirtual\b\s*//' "$outfile"

    # remove undesired UIM
    $SED -r -i '/^[ \t]*#define[ \t]+FAUST_(FILE_NAME|CLASS_NAME|INPUTS|OUTPUTS|ACTIVES|PASSIVES)/d' "$outfile"
    $SED -r -i '/^[ \t]*FAUST_ADD.*/d' "$outfile"

    # direct access to parameter variables
    $SED -r -i 's/\bprivate:/public:/' "$outfile"

    # remove trailing whitespace
    $SED -r -i 's/[ \t]+$//' "$outfile"
}

faustgen
