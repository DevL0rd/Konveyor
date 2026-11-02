.pragma library

function collect(item, text, found) {
    if (!item || !item.visible)
        return found
    if (item.title === text || item.label === text || item.text === text)
        found.push(item)
    const children = item.children || []
    for (let i = 0; i < children.length; ++i)
        collect(children[i], text, found)
    return found
}

function pending(item) {
    if (!item || !item.visible)
        return false
    if (item.asynchronous === true && item.status === 2)
        return true
    const children = item.children || []
    for (let i = 0; i < children.length; ++i) {
        if (pending(children[i]))
            return true
    }
    return false
}
