.pragma library
.import "index/LayoutIndex.js" as LayoutIndex
.import "index/LookIndex.js" as LookIndex
.import "index/MotionIndex.js" as MotionIndex
.import "index/MouseIndex.js" as MouseIndex
.import "index/ShortcutsIndex.js" as ShortcutsIndex
.import "index/RulesIndex.js" as RulesIndex
.import "index/MonitorsIndex.js" as MonitorsIndex
.import "index/WorkspacesIndex.js" as WorkspacesIndex
.import "index/PlasmaIndex.js" as PlasmaIndex

const entries = [].concat(
    LayoutIndex.entries, LookIndex.entries, MotionIndex.entries, MouseIndex.entries, ShortcutsIndex.entries,
    RulesIndex.entries, MonitorsIndex.entries, WorkspacesIndex.entries, PlasmaIndex.entries);

function search(query) {
    const words = query.toLowerCase().split(/\s+/).filter(Boolean);
    return entries.filter(entry => {
        const haystack = (entry.label + " " + (entry.section || "") + " " + (entry.keywords || "")).toLowerCase();
        return words.every(word => haystack.includes(word));
    }).slice(0, 30);
}
