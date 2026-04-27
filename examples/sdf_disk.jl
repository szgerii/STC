begin
    @gl_const global MOUSE_EPS = 0.001

    @gl_out global frag_col::Vec4
    @gl_uniform global mouse::Vec4
    @gl_uniform global resolution::IVec2

    function f()
        vec3(0) .* Vec3(cos(0))
    end

    function main()
        p = (2.0 * gl_FragCoord[:xy] - resolution["xy"]) ./ resolution["y"]
        m = (2.0 * mouse["xy"] - resolution["xy"]) ./ resolution["y"]

        d = length(p) - 0.5

        while true
            h = vec3(0)
        end

        local col::Vec3
        if d > 0.0
            col = vec3(0.9, 0.6, 0.3)
        else
            col = vec3(0.65, 0.85, 1.0)
        end

        col *= 1.0 - exp(-6.0 * abs(d))
        col *= 0.8 + 0.2 * cos(150 * d)
        col = mix(col, Vec3(1), 1.0 - smoothstep(0.0, 0.01, abs(d)))

        if (mouse["z"] > MOUSE_EPS)
            d = length(m) - 0.5
            col = mix(col, Vec3(1, 1, 0), 1.0 - smoothstep(0.0, 0.005, abs(length(p - m) - abs(d)) - 0.0025))
            col = mix(col, Vec3(1, 1, 0), 1.0 - smoothstep(0.0, 0.005, length(p - m) - 0.015))
        end

        frag_col = Vec4(col, 1.0)
    end
end
