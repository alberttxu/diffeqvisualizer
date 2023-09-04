include("compute.jl")

function main()
    println(solve_autonomous([1.0,0], Float64[0 1; -1 0], 0.2))
end

f(x) = x

g(x) = x

h(x) = x[1] + x[2]