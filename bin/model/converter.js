if (process.argv.length < 4) {
    console.log("Usage: node convert.js <source.json> <destination>")
    return
}

const fs = require('fs')

const modelJSON = JSON.parse(fs.readFileSync(process.argv[2]))
const isNormalIn = "normal" in modelJSON
const isUVIn = "uv" in modelJSON
const vtxCount = Number(modelJSON["vertex"])
const idxCount = Number(modelJSON["index"].length)

let flag = 0
let sizePerVertex = 3

if (isNormalIn) {
    flag |= 1
    sizePerVertex += 3
}
if (isUVIn) {
    flag |= 2
    sizePerVertex += 2
}

const bufferSize = 4 * (1 + 1 + sizePerVertex * vtxCount + 1 + idxCount)
const buffer = Buffer.alloc(bufferSize)

let offset = 0

buffer.writeUInt32LE(flag, offset)
offset += 4

buffer.writeUInt32LE(vtxCount, offset)
offset += 4

for (let i = 0; i < vtxCount; ++i) {
    buffer.writeFloatLE(Number(modelJSON["position"][3 * i + 0]), offset)
    offset += 4
    buffer.writeFloatLE(Number(modelJSON["position"][3 * i + 1]), offset)
    offset += 4
    buffer.writeFloatLE(Number(modelJSON["position"][3 * i + 2]), offset)
    offset += 4
    if (isNormalIn) {
        buffer.writeFloatLE(Number(modelJSON["normal"][3 * i + 0]), offset)
        offset += 4
        buffer.writeFloatLE(Number(modelJSON["normal"][3 * i + 1]), offset)
        offset += 4
        buffer.writeFloatLE(Number(modelJSON["normal"][3 * i + 2]), offset)
        offset += 4
    }
    if (isUVIn) {
        buffer.writeFloatLE(Number(modelJSON["uv"][2 * i + 0]), offset)
        offset += 4
        buffer.writeFloatLE(Number(modelJSON["uv"][2 * i + 1]), offset)
        offset += 4
    }
}

buffer.writeUInt32LE(idxCount, offset)
offset += 4

for (let i = 0; i < idxCount; ++i) {
    buffer.writeUInt32LE(Number(modelJSON["index"][i]), offset)
    offset += 4
}

if (offset === bufferSize) {
    fs.writeFileSync(process.argv[3], buffer)
    console.log("[ info ] converter.js: " + process.argv[3] + " is generated from " + process.argv[2])
} else {
    console.error("[ error ] converter.js: something is wrong with reading data.")
}
