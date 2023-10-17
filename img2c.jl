if length(ARGS) == 0
    println("usage: \$ julia img2c.jl input.png")
    exit(1)
end

imgpath = ARGS[1]
@assert isfile(imgpath)

using Images

# img = load("./assets/xdoteqAx.png")
img = load(imgpath)

imgname = split(basename(imgpath), ".")[1]
outputfile = open("$(imgname).c", "w")

write(outputfile, "unsigned char img[$(size(img)[1])][$(4 * size(img)[2])] = {\n")

for i in 1:size(img)[1]
    write(outputfile, "{")

    rowpixelvals = UInt8[]
    for j in 1:size(img)[2]
        push!(rowpixelvals, reinterpret(img[i,j].r))
        push!(rowpixelvals, reinterpret(img[i,j].g))
        push!(rowpixelvals, reinterpret(img[i,j].b))
        push!(rowpixelvals, reinterpret(img[i,j].alpha))
    end
    write(outputfile, join(rowpixelvals, ", "))

    if i == size(img)[1]
        write(outputfile, "}\n};")
    else
        write(outputfile, "},\n")
    end
end

close(outputfile)
