.pragma library

function bezierY(x1, y1, x2, y2, x) {
    const coordinate = (a, b, t) => 3 * a * t * (1 - t) * (1 - t) + 3 * b * t * t * (1 - t) + t * t * t;
    let low = 0;
    let high = 1;
    let t = x;
    for (let i = 0; i < 40; ++i) {
        const value = coordinate(x1, x2, t);
        if (Math.abs(value - x) < 1e-5) {
            break;
        }
        if (value < x) {
            low = t;
        } else {
            high = t;
        }
        t = (low + high) / 2;
    }
    return coordinate(y1, y2, t);
}

function easingAt(curve, bezier, t) {
    const x = Math.min(1, Math.max(0, t));
    switch (curve) {
    case "ease-out-quad":
        return 1 - (1 - x) * (1 - x);
    case "ease-out-cubic":
        return 1 - Math.pow(1 - x, 3);
    case "ease-out-expo":
        return 1 - Math.pow(2, -10 * x);
    case "cubic-bezier":
        return bezierY(bezier[0], bezier[1], bezier[2], bezier[3], x);
    default:
        return x;
    }
}

function springAt(dampingRatio, stiffness, seconds) {
    const omega0 = Math.sqrt(stiffness);
    const beta = dampingRatio * omega0;
    const x0 = -1;
    const envelope = Math.exp(-beta * seconds);
    const b = beta * x0;
    if (Math.abs(beta - omega0) < 1e-6) {
        return 1 + envelope * (x0 + b * seconds);
    }
    if (beta < omega0) {
        const omega = Math.sqrt(omega0 * omega0 - beta * beta);
        return 1 + envelope * (x0 * Math.cos(omega * seconds) + (b / omega) * Math.sin(omega * seconds));
    }
    const omega = Math.sqrt(beta * beta - omega0 * omega0);
    return 1 + envelope * (x0 * Math.cosh(omega * seconds) + (b / omega) * Math.sinh(omega * seconds));
}

function springDurationMs(dampingRatio, stiffness, epsilon) {
    const beta = dampingRatio * Math.sqrt(stiffness);
    if (beta <= 0) {
        return 10000;
    }
    const envelopeTime = -Math.log(epsilon) / beta;
    if (dampingRatio <= 1) {
        return Math.min(10000, envelopeTime * 1000);
    }
    for (let seconds = 0; seconds < 10; seconds += 0.002) {
        if (Math.abs(1 - springAt(dampingRatio, stiffness, seconds)) <= epsilon) {
            return seconds * 1000;
        }
    }
    return 10000;
}

function motion(params) {
    if (!params) {
        return { durationMs: 0, valueAt: () => 1 };
    }
    if (params.kind === "spring") {
        const damping = params["damping-ratio"];
        const stiffness = Math.max(1, params.stiffness);
        return {
            durationMs: springDurationMs(damping, stiffness, params.epsilon),
            valueAt: ms => springAt(damping, stiffness, ms / 1000)
        };
    }
    const duration = Math.max(1, params["duration-ms"]);
    return {
        durationMs: duration,
        valueAt: ms => easingAt(params.curve, params.bezier || [0, 0, 1, 1], ms / duration)
    };
}
