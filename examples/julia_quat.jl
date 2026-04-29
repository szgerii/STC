
@gl_out global frag_col::Vec4
@gl_uniform global time::Float32
@gl_uniform global resolution::IVec2
@gl_uniform global mouse::Vec4

function hash3(n::Float32)
    fract(sin(vec3(n, n + 1.0, n + 2.0)) .* vec3(43758.5453123, 22578.1459123, 19642.3490423))
end

function qSquare(a::Vec4)
    vec4(a[:x] * a[:x] - dot(a[:yzw], a[:yzw]), 2.0 * a[:x] .* a[:yzw])
end

function qCube(a::Vec4)
    a .* (4.0 * a[:x] * a[:x] .- dot(a, a) .* vec4(3.0, 1.0, 1.0, 1.0))
end

function lengthSquared(z::Vec4)
    dot(z, z)
end

function map_(p::Vec3, c::Vec4)
    z = vec4(p, 0.2)

    m2 = 0.0
    t = vec2(1e10)

    dz2 = 1.0
    i = 0
    while i < 10
        dz2 *= 9.0 * lengthSquared(qSquare(z))

        z = qCube(z) + c

        m2 = dot(z, z)

        if m2 > 10000.0
            break
        end

        t = min(t, vec2(m2, abs(z[:x])))

        i += 1
    end

    d = 0.25 * log(m2) * sqrt(m2 / dz2)

    vec3(d, t)
end

function raycast(ro::Vec3, rd::Vec3, c::Vec4)
    maxd = 8.0
    precis = 0.002
    h = 1.0
    t = 0.0
    d = 0.0
    m = 1.0

    i = 0
    while i < 150
        if h < precis || t > maxd
            break
        end

        t += h
        res = map_(ro + rd * t, c)
        h = res[:x]
        d = res[:y]
        m = res[:z]

        i += 1
    end

    if t > maxd
        m = -1.0
    end

    vec3(t, d, m)
end

function calcNormal(pos::Vec3, e::Float32, c::Vec4)
    eps = vec3(e, 0.0, 0.0)

    normalize(vec3(
        map_(pos + eps[:xyy], c)[:x] - map_(pos - eps[:xyy], c)[:x],
        map_(pos + eps[:yxy], c)[:x] - map_(pos - eps[:yxy], c)[:x],
        map_(pos + eps[:yyx], c)[:x] - map_(pos - eps[:yyx], c)[:x],
    ))
end

function calcPixel(_pi::Vec2, _time::Float32)
    c = vec4(-0.1, 0.6, 0.9, -0.3) + 0.1 * sin(vec4(3, 0, 1, 2) + 0.5 * vec4(1.0, 1.3, 1.7, 2.1) * _time)

    q = _pi ./ resolution[:xy]
    p = -1.0 .+ 2.0 * q
    p[:x] *= Float32(resolution[:x]) / Float32(resolution[:y])

    m = vec2(0.5)
    if mouse[:z] > 0.0
        m = mouse[:xy] ./ resolution[:xy]
    end

    an = -2.4 + 0.2 * _time - 6.2 * m[:x]
    ro = 4.0 * vec3(sin(an), 0.25, cos(an))
    ta = vec3(0.0, 0.08, 0.0)
    ww = normalize(ta - ro)
    uu = normalize(cross(ww, vec3(0, 1, 0)))
    vv = normalize(cross(uu, ww))
    rd = normalize(p[:x] * uu + p[:y] * vv + 4.1 * ww)

    tmat = raycast(ro, rd, c)

    col = vec3(0)
    if tmat[:z] > -0.5
        pos = ro + tmat[:x] * rd
        _nor = calcNormal(pos, 0.001, c)
        sor = calcNormal(pos, 0.01, c)

        mate = 0.5 .+ 0.5 * sin(tmat[:z] * 4.0 .+ 4.0 .+ vec3(3.0, 1.5, 2.0) + _nor * 0.2)[:xzy]

        occ = clamp(tmat[:y] * 0.5 + 0.5 * (tmat[:y] * tmat[:y]), 0.0, 1.0) * (1.0 + 0.1 * _nor[:y])

        col = vec3(0)
        i = 0
        while i < 32
            rr = normalize(-1.0 .+ 2.0 * hash3(Float32(i) * 123.5463))
            rr = normalize(_nor + 8.0 * rr)
            rr = rr * sign(dot(_nor, rr))
            col += pow(vec3(0.8, 0.3, 0.1), vec3(2.2)) * dot(rr, _nor)

            i += 1
        end

        col = 5.0 * occ * (col / 32.0)

        col = col .* (1.0 .+ 1.0 * pow(clamp(1.0 + dot(rd, sor), 0.0, 1.0), 1.0) * vec3(1))

        fre = pow(clamp(1.0 + dot(rd, sor), 0.0, 1.0), 5.0)
        ref = reflect(rd, _nor)
        col *= 1.0 - 0.5 * fre
        col += 1.5 * (0.5 + 0.5 * fre) * pow(vec3(vec3(0.7, 0.3, 0.1)), vec3(2.0)) * occ

        col = col .* mate
    else
        col = vec3(0)
    end

    col = pow(clamp(col, 0.0, 1.0), vec3(0.45))

    col
end

function main()
    samples = 4
    col = vec3(0.0)

    i = 0
    while i < samples
        r = 0.35234
        h = hash3(r + Float32(i) + time)
        p = gl_FragCoord[:xy] + h[:xy]
        t = time + 0.5 * h[:z] / 24.0
        col += calcPixel(p, t)

        i += 1
    end

    global frag_col::Vec4
    frag_col = vec4(col / Float32(samples), 1.0)
end
