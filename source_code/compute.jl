using LinearAlgebra

function solve_autonomous(x, A::Matrix{Float64}, t::Float64)
    return exp(t*A) * x
end

# I couldn't figure out how to check if the element type is real or complex via the c API.
# So this wrapper just converts the resulting arrays to have complex elements.
function eigen_ComplexF64(A::Matrix{Float64})
    F = eigen(A)
    return Eigen(convert.(ComplexF64, F.values), convert.(ComplexF64, F.vectors))
end

function eigencompose(λ::Vector{Complex{Float64}}, V::Matrix{Complex{Float64}})
    return V * diagm(λ) * inv(V)
end

function changeEigenvalues(A::Matrix{Float64}, newλ::Vector{Complex{Float64}})
    return eigencompose(newλ, eigen_ComplexF64(A).vectors)
end

function changeEigenvalues_wrapper(A::Matrix{Float64}, newλ::Vector{Float64}) :: Matrix{Float64}
    @assert length(newλ) == 4
    newλ_complex = [newλ[1] + newλ[2]im;
                    newλ[3] + newλ[4]im]
    return changeEigenvalues(A, newλ_complex)
end

# warm start the jit
solve_autonomous(Float64[0, 0], Float64[0 0; 0 0], 0.0)
eigen(Float64[1 0; 0 1])
A = Float64[1 0; 0 1]
newλ = [2.0 + 0im; 3.0 + 0im]
B = changeEigenvalues(A, newλ)
# @show B
@assert isapprox(B, Float64[2 0; 0 3])
newλsep = [2.0, 0.0, 3.0, 0.0]
C = changeEigenvalues_wrapper(A, newλsep)
# @show C
@assert isapprox(C, Float64[2 0; 0 3])
