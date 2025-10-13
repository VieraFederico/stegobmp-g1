#!/bin/bash

# Fast build script for stegobmp project
# Hardcoded paths for maximum speed

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Cool ASCII art banner
echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                                                              ║"
echo "║    ███████╗████████╗███████╗ ██████╗ ██████╗ ██████╗ ███╗   ███╗██████╗  ║"
echo "║    ██╔════╝╚══██╔══╝██╔════╝██╔═══██╗██╔══██╗██╔══██╗████╗ ████║██╔══██╗ ║"
echo "║    ███████╗   ██║   █████╗  ██║   ██║██████╔╝██████╔╝██╔████╔██║██████╔╝ ║"
echo "║    ╚════██║   ██║   ██╔══╝  ██║   ██║██╔══██╗██╔═══╝ ██║╚██╔╝██║██╔═══╝  ║"
echo "║    ███████║   ██║   ███████╗╚██████╔╝██████╔╝██████╔╝██║ ╚═╝ ██║██████╔╝ ║"
echo "║    ╚══════╝   ╚═╝   ╚══════╝ ╚═════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═════╝  ║"
echo "║                                                              ║"
echo "║                     BUILDING STEGANOGRAPHY TOOL          ║"
echo "║                                                              ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

echo -e "${YELLOW} Starting ultra-fast compilation...${NC}"
echo ""

# Compile all source files directly with progress indicators
echo -e "${BLUE} Compiling source files...${NC}"

echo -e "${WHITE}   main.c${NC}"
gcc -Wall -Wextra -O2 -c src/main.c -o src/main.o

echo -e "${WHITE}   bmp_handler.c${NC}"
gcc -Wall -Wextra -O2 -c src/bmp_handler/bmp_handler.c -o src/bmp_handler/bmp_handler.o

echo -e "${WHITE}   lsb1.c${NC}"
gcc -Wall -Wextra -O2 -c src/lsb1/lsb1.c -o src/lsb1/lsb1.o

echo -e "${WHITE}   lsb4.c${NC}"
gcc -Wall -Wextra -O2 -c src/lsb4/lsb4.c -o src/lsb4/lsb4.o

echo -e "${WHITE}   lsbi.c${NC}"
gcc -Wall -Wextra -O2 -c src/lsbi/lsbi.c -o src/lsbi/lsbi.o

echo -e "${WHITE}   file_management.c${NC}"
gcc -Wall -Wextra -O2 -c src/utils/file_management/file_management.c -o src/utils/file_management/file_management.o

echo -e "${WHITE}   parser.c${NC}"
gcc -Wall -Wextra -O2 -c src/utils/parser/parser.c -o src/utils/parser/parser.o

echo -e "${WHITE}   translator.c${NC}"
gcc -Wall -Wextra -O2 -c src/utils/translator/translator.c -o src/utils/translator/translator.o

echo ""
echo -e "${PURPLE} Linking everything together...${NC}"

# Link everything together
gcc -Wall -Wextra -O2 -o stegobmp \
    src/main.o \
    src/bmp_handler/bmp_handler.o \
    src/lsb1/lsb1.o \
    src/lsb4/lsb4.o \
    src/lsbi/lsbi.o \
    src/utils/file_management/file_management.o \
    src/utils/parser/parser.o \
    src/utils/translator/translator.o \
    -lssl -lcrypto

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║   BUILD COMPLETE! Executable: ${WHITE}stegobmp${GREEN}                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║   Ready to hide secrets in BMP files!                     ║${NC}"
echo -e "${GREEN}║   Supports: LSB1, LSB4, LSBI steganography                ║${NC}"
echo -e "${GREEN}║    Includes: AES encryption support                      ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}                    COMMAND LINE OPTIONS                      ${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${WHITE}REQUIRED PARAMETERS:${NC}"
echo -e "${YELLOW}  -embed${NC}                    Enable embedding mode"
echo -e "${YELLOW}  -extract${NC}                  Enable extraction mode"
echo -e "${YELLOW}  -in <file>${NC}               Input file to hide (embed mode only)"
echo -e "${YELLOW}  -p <bitmapfile>${NC}          Carrier BMP file"
echo -e "${YELLOW}  -out <bitmapfile>${NC}        Output BMP file"
echo -e "${YELLOW}  -steg <method>${NC}           Steganography method: LSB1, LSB4, LSBI"
echo ""
echo -e "${WHITE}OPTIONAL PARAMETERS:${NC}"
echo -e "${YELLOW}  -a <algorithm>${NC}           Encryption algorithm: aes128, aes192, aes256, 3des"
echo -e "${YELLOW}  -m <mode>${NC}                Encryption mode: ecb, cfb, ofb, cbc"
echo -e "${YELLOW}  -pass <password>${NC}         Encryption password"
echo ""
echo -e "${WHITE}USAGE EXAMPLES:${NC}"
echo ""
echo -e "${GREEN}EMBED (Hide file in BMP):${NC}"
echo -e "${YELLOW}  ./stegobmp -embed -in secret.txt -p carrier.bmp -out hidden.bmp -steg LSB1${NC}"
echo -e "${YELLOW}  ./stegobmp -embed -in document.pdf -p image.bmp -out result.bmp -steg LSB4${NC}"
echo -e "${YELLOW}  ./stegobmp -embed -in data.bin -p photo.bmp -out encrypted.bmp -steg LSBI -a aes256 -m cbc -pass mypassword${NC}"
echo ""
echo -e "${GREEN}EXTRACT (Recover hidden file):${NC}"
echo -e "${YELLOW}  ./stegobmp -extract -p hidden.bmp -out recovered.txt -steg LSB1${NC}"
echo -e "${YELLOW}  ./stegobmp -extract -p result.bmp -out document.pdf -steg LSB4${NC}"
echo -e "${YELLOW}  ./stegobmp -extract -p encrypted.bmp -out data.bin -steg LSBI -a aes256 -m cbc -pass mypassword${NC}"
echo ""
echo -e "${WHITE}STEGANOGRAPHY METHODS:${NC}"
echo -e "${CYAN}  LSB1${NC}  - Least Significant Bit (1 bit per pixel)"
echo -e "${CYAN}  LSB4${NC}  - Least Significant Bits (4 bits per pixel)"
echo -e "${CYAN}  LSBI${NC}  - LSB Enhanced (improved algorithm)"
echo ""
echo -e "${WHITE}ENCRYPTION ALGORITHMS:${NC}"
echo -e "${CYAN}  aes128${NC} - AES 128-bit encryption"
echo -e "${CYAN}  aes192${NC} - AES 192-bit encryption"
echo -e "${CYAN}  aes256${NC} - AES 256-bit encryption"
echo -e "${CYAN}  3des${NC}   - Triple DES encryption"
echo ""
echo -e "${WHITE}ENCRYPTION MODES:${NC}"
echo -e "${CYAN}  ecb${NC}    - Electronic Codebook"
echo -e "${CYAN}  cfb${NC}    - Cipher Feedback"
echo -e "${CYAN}  ofb${NC}    - Output Feedback"
echo -e "${CYAN}  cbc${NC}    - Cipher Block Chaining"
echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo ""
