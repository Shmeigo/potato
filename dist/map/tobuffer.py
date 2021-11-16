from PIL import Image
import sys
import struct

with Image.open(sys.argv[1]) as im:
    pixels = im.load()
    width, height = im.size
    print(im.size)
    smallest = pixels[0, 0][0]
    largest = pixels[0, 0][0]
    lt = []
    buffer = bytes()
    for i in range(width):
        for j in range(height):
            curr = pixels[i, j][0]
            smallest = smallest if smallest < curr else curr
            largest = largest if largest > curr else curr
    factor = 1.0/ (largest - smallest)
    for i in range(width):
        for j in range(height):
            curr = pixels[i, j][0]
            curr -= smallest
            curr *= factor
            lt.append(float(curr))
    
    print(smallest)
    print(largest)
    print(len(lt))
    with open(sys.argv[1].split(".")[0] + ".txt", "w") as handle:
        handle.write("\n".join(map(str, lt)))