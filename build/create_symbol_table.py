import sys

def error(message): 
    print(message)
    sys.exit(1)


def main():
    outfile = open(sys.argv[1], 'wb')

    for line in sys.stdin:
        line = line.strip()
        words = line.split(' ')  
        if (len(words) != 3):
            error(f"bad line {line}")

        num_str = words[0]
        t = words[1]
        name = words[2]

        num = int(num_str, 16)

        # byte length 8 is used here, it could be more efficient to only use 4 on 32 bit  
        num_bytes = num.to_bytes(8, byteorder = 'little') # TODO: change this if big endian

        name_bytes = name.encode('ascii')
        name_bytes += bytes([0]) # add null terminator

        # print(num_bytes + name_bytes)
        outfile.write(num_bytes + name_bytes)

    outfile.write('____SYMTABLE_DONE'.encode('ascii'))

        
if __name__ == '__main__':
    main()
