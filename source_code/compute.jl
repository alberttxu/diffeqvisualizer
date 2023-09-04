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
