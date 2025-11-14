#!/bin/bash

# Comprehensive test script for stegobmp
# Tests all embed/extract combinations and extracts from provided test files

# Don't exit on error - we want to continue testing even if one fails

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Directories (relative to script location)
TEST_INPUT_DIR="test/input"
TEST_OUTPUT_DIR="test/output"
STEGOBMP="./stegobmp"

# Test files
CARRIER="$TEST_INPUT_DIR/lado.bmp"
SECRET_FILE="$TEST_INPUT_DIR/testSecret.txt"
PASSWORD="margarita"

# Log file
LOG_FILE="$TEST_OUTPUT_DIR/test.log"
mkdir -p "$TEST_OUTPUT_DIR"

# Initialize log
echo "=== STEGOBMP TEST SCRIPT ===" > "$LOG_FILE"
echo "Started: $(date)" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Function to log messages
log() {
    echo -e "$1" | tee -a "$LOG_FILE"
}

# Function to log success
log_success() {
    log "${GREEN}✓ SUCCESS:${NC} $1"
    echo "SUCCESS: $1" >> "$LOG_FILE"
}

# Function to log failure
log_failure() {
    log "${RED}✗ FAILURE:${NC} $1"
    echo "FAILURE: $1" >> "$LOG_FILE"
}

# Function to log info
log_info() {
    log "${BLUE}ℹ INFO:${NC} $1"
    echo "INFO: $1" >> "$LOG_FILE"
}

# Function to log debug
log_debug() {
    log "${CYAN}DEBUG:${NC} $1"
    echo "DEBUG: $1" >> "$LOG_FILE"
}

# Function to check if files are equal
check_files_equal() {
    local file1="$1"
    local file2="$2"
    if cmp -s "$file1" "$file2" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to test embed/extract combination
test_embed_extract() {
    local steg_method="$1"
    local algo="$2"  # empty, aes128, aes256, 3des
    local mode="$3"  # empty, cbc, ofb, cfb
    local test_name="${steg_method}"
    
    if [ -n "$algo" ]; then
        test_name="${test_name}_${algo}_${mode}"
    fi
    
    log_info "Testing embed/extract: $test_name"
    
    local embed_out="$TEST_OUTPUT_DIR/embed_${test_name}.bmp"
    local extract_out="$TEST_OUTPUT_DIR/extract_${test_name}.txt"
    
    # Build embed command
    local embed_cmd="$STEGOBMP -embed -in \"$SECRET_FILE\" -p \"$CARRIER\" -out \"$embed_out\" -steg $steg_method"
    if [ -n "$algo" ]; then
        embed_cmd="$embed_cmd -a $algo -m $mode -pass \"$PASSWORD\""
    fi
    
    log_debug "Embed command: $embed_cmd"
    
    # Execute embed
    if eval "$embed_cmd" >> "$LOG_FILE" 2>&1; then
        log_debug "Embed succeeded for $test_name"
    else
        log_failure "Embed failed for $test_name"
        return 1
    fi
    
    # Build extract command
    local extract_cmd="$STEGOBMP -extract -p \"$embed_out\" -out \"$extract_out\" -steg $steg_method"
    if [ -n "$algo" ]; then
        extract_cmd="$extract_cmd -a $algo -m $mode -pass \"$PASSWORD\""
    fi
    
    log_debug "Extract command: $extract_cmd"
    
    # Execute extract
    if eval "$extract_cmd" >> "$LOG_FILE" 2>&1; then
        log_debug "Extract succeeded for $test_name"
    else
        log_failure "Extract failed for $test_name"
        return 1
    fi
    
    # Verify extracted file matches original
    if check_files_equal "$SECRET_FILE" "$extract_out"; then
        log_success "File verification passed for $test_name"
        return 0
    else
        log_failure "File verification failed for $test_name (files differ)"
        if [ -f "$extract_out" ]; then
            log_debug "Original file size: $(wc -c < "$SECRET_FILE" 2>/dev/null || echo "unknown") bytes"
            log_debug "Extracted file size: $(wc -c < "$extract_out" 2>/dev/null || echo "unknown") bytes"
        else
            log_debug "Extracted file does not exist"
        fi
        return 1
    fi
}

# Function to extract from test BMP file
extract_from_test_bmp() {
    local bmp_file="$1"
    local steg_method="$2"
    local algo="$3"
    local mode="$4"
    local expected_ext=".png"
    
    local bmp_name=$(basename "$bmp_file" .bmp)
    local extract_out="$TEST_OUTPUT_DIR/extract_${bmp_name}${expected_ext}"
    
    log_info "Extracting from: $bmp_name"
    
    # Build extract command
    local extract_cmd="$STEGOBMP -extract -p \"$bmp_file\" -out \"$extract_out\" -steg $steg_method"
    if [ -n "$algo" ]; then
        extract_cmd="$extract_cmd -a $algo -m $mode -pass \"$PASSWORD\""
    fi
    
    log_debug "Extract command: $extract_cmd"
    
    # Execute extract
    if eval "$extract_cmd" >> "$LOG_FILE" 2>&1; then
        if [ -f "$extract_out" ]; then
            local file_size=$(wc -c < "$extract_out" 2>/dev/null || echo "unknown")
            log_success "Extracted $bmp_name -> $extract_out (${file_size} bytes)"
            
            # Verify it's a valid PNG (check magic bytes)
            if file "$extract_out" 2>/dev/null | grep -q "PNG"; then
                log_success "File $extract_out is a valid PNG"
            else
                # Try hexdump check
                if head -c 8 "$extract_out" 2>/dev/null | od -An -tx1 | grep -q "89 50 4e 47"; then
                    log_success "File $extract_out is a valid PNG (magic bytes check)"
                else
                    log_failure "File $extract_out is not a valid PNG"
                    log_debug "File type: $(file "$extract_out" 2>/dev/null || echo "unknown")"
                    log_debug "First bytes: $(head -c 16 "$extract_out" 2>/dev/null | od -An -tx1 || echo "unknown")"
                fi
            fi
            return 0
        else
            log_failure "Extract succeeded but output file not found: $extract_out"
            return 1
        fi
    else
        log_failure "Extract failed for $bmp_name"
        return 1
    fi
}

# Check if stegobmp exists
if [ ! -f "$STEGOBMP" ]; then
    log_failure "stegobmp executable not found at $STEGOBMP"
    log_info "Please build the project first: make"
    exit 1
fi

# Check if test files exist
if [ ! -f "$CARRIER" ]; then
    log_failure "Carrier file not found: $CARRIER"
    exit 1
fi

if [ ! -f "$SECRET_FILE" ]; then
    log_failure "Secret file not found: $SECRET_FILE"
    exit 1
fi

log_info "Starting comprehensive tests..."
log_info "Carrier: $CARRIER"
log_info "Secret file: $SECRET_FILE"
log_info "Password: $PASSWORD"
echo ""

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# ============================================================================
# PART 1: Test all embed/extract combinations with lado.bmp
# ============================================================================
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
log "${CYAN}PART 1: Testing Embed/Extract Combinations${NC}"
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Test without encryption
for method in LSB1 LSB4 LSBI; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if test_embed_extract "$method" "" ""; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo ""
done

# Test with AES128/CBC
for method in LSB1 LSB4 LSBI; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if test_embed_extract "$method" "aes128" "cbc"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo ""
done

# Test with AES256/OFB
for method in LSB1 LSB4 LSBI; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if test_embed_extract "$method" "aes256" "ofb"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo ""
done

# Test with 3DES/CFB (note: parser uses "3des" and "cfb", not "des" and "cfb8")
for method in LSB1 LSB4 LSBI; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if test_embed_extract "$method" "3des" "cfb"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo ""
done

# ============================================================================
# PART 2: Extract from provided test BMP files
# ============================================================================
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
log "${CYAN}PART 2: Extracting from Provided Test Files${NC}"
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# ladoLSB1.bmp - LSB1, no encryption
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSB1.bmp" "LSB1" "" ""; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ladoLSB4.bmp - LSB4, no encryption
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSB4.bmp" "LSB4" "" ""; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ladoLSBI.bmp - LSBI, no encryption
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSBI.bmp" "LSBI" "" ""; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ladoLSB1aes128cbc.bmp - LSB1, AES128, CBC
TOTAL_TESTS=$((TOTAL_TESTS + 1))
log_debug "Expected key (32 bytes): 03db0a157acfe8de523760aa731d8122"
log_debug "Expected IV (16 bytes): 212420edc583a686a94d19a3497363a2"
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSB1aes128cbc.bmp" "LSB1" "aes128" "cbc"; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ladoLSBIaes256ofb.bmp - LSBI, AES256, OFB
TOTAL_TESTS=$((TOTAL_TESTS + 1))
log_debug "Expected key (32 bytes): 03db0a157acfe8de523760aa731d8122b25f8d99f3173ec0b52849f459a4c20d"
log_debug "Expected IV (16 bytes): 212420edc583a686a94d19a3497363a2"
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSBIaes256ofb.bmp" "LSBI" "aes256" "ofb"; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ladoLSBIdescfb.bmp - LSBI, 3DES, CFB (note: parser uses "3des" and "cfb")
TOTAL_TESTS=$((TOTAL_TESTS + 1))
log_debug "Expected key (24 bytes): 03db0a157acfe8de523760aa731d8122b25f8d99f3173ec0"
log_debug "Key breakdown: k1=03db0a157acfe8de k2=523760aa731d8122 k3=b25f8d99f3173ec0"
log_debug "Expected IV (8 bytes): b52849f459a4c20d"
if extract_from_test_bmp "$TEST_INPUT_DIR/ladoLSBIdescfb.bmp" "LSBI" "3des" "cfb"; then
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi
echo ""

# ============================================================================
# SUMMARY
# ============================================================================
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
log "${CYAN}TEST SUMMARY${NC}"
log "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
log "Total tests: $TOTAL_TESTS"
log "${GREEN}Passed: $PASSED_TESTS${NC}"
log "${RED}Failed: $FAILED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    log_success "All tests passed!"
    echo "All tests passed!" >> "$LOG_FILE"
    exit 0
else
    log_failure "Some tests failed. Check $LOG_FILE for details."
    echo "Some tests failed." >> "$LOG_FILE"
    exit 1
fi

