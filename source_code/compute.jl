using LinearAlgebra

function solve_autonomous(x, A::Matrix{Float64}, t::Float64)
    return exp(t*A) * x
end