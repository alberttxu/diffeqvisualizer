using LinearAlgebra

function solve_autonomous(x0, A::Matrix{Float64}, t::Float64)
    return exp(t*A) * x0
end

function main()
    A = Float64[
         0 1;
        -1 0]

    for t in 0:0.1:2
        println("t = $t, A = ", evolve(A, t))
    end
end


f(x) = x

g(x) = x

h(x) = x[1] + x[2]

