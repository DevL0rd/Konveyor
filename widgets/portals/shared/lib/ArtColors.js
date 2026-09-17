.pragma library

var cache = {}

function get(key) {
    return cache[key]
}

function put(key, color) {
    cache[key] = color
}
