# node-libeth
Node bindings for generating Ethash Epoch context and light cache

Usage

```
// arg1: epoch generation number. Ex: 460
var ecs = ethlib.getEpochContextBin(460) // returns Object { }
var ec = JSON.parse(ecs)
console.log("ectx:", ec)

// Output:
/**
ectx: {
  bin: [
    '0xcc010000f92f0b008000e00300000000c0feab0600000000f5ff6501000000000000000000000000'
  ],
  lightNumItems: 733177,
  lightSize: 46923328,
  dagSize: 3003120256,
  dagNumItems: 23461877
}
**/
```

```
var lcBuf = ethlib.getLightCache() // returns Buffer [ ... ]
console.log("lc:", lcBuf)

// Output:
/**
lc: <Buffer 8f f0 f8 2f f1 4b 4d 57 d4 a1 f7 90 da 64 d3 b8 32 0b de 09 cb da 7f aa cd e0 dd 1d 0d 4b 30 14 e5 a9 11 90 2e b2 ab 3b 0b 61 76 80 c8 5e 73 ba 62 d8 ... 46923278 more bytes>
**/
```

