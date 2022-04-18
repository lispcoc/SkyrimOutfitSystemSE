const fs = require('fs')
let text_jp = fs.readFileSync("skyrimoutfitsystem_japanese.txt", "utf16le")
let text_en = fs.readFileSync("skyrimoutfitsystem_english.txt", "utf16le")
let result = ""
var pair = {}
text_jp.split("\r\n").forEach(l => {
    var a = l.split("\t");
    if (a.length > 1) {
        pair[a[0]] = a[1]
    }
})

text_en.split("\r\n").forEach(l => {
    var a = l.split("\t");
    if (a.length > 1) {
        if (pair[a[0]]) {
            result += a[0] + "\t" + pair[a[0]]
        } else {
            result += l
        }
    } else {
        result += l
    }
    result += "\r\n"
})
console.log(result)
fs.writeFileSync("out.txt", result)