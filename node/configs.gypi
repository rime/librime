{
  "targets": [{
    "target_name": "configs",
    "type": "none",
    "copies": [{
      "destination": "<(PRODUCT_DIR)",
      "files": [
        "../data/config/s2t.json",
        "../data/config/s2tw.json",
        "../data/config/s2twp.json",
        "../data/config/t2s.json",
        "../data/config/tw2s.json",
        "../data/config/tw2sp.json",
      ]
    }]
  }]
}
