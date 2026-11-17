// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/utilities.h (version comparison).

/// Compare two dotted version strings.
pub fn compare_version_string(x: &str, y: &str) -> i32 {
    let xs: Vec<&str> = x.split('.').collect();
    let ys: Vec<&str> = y.split('.').collect();
    let m = xs.len().max(ys.len());
    for i in 0..m {
        let v1 = xs.get(i).and_then(|s| s.parse::<i32>().ok()).unwrap_or(0);
        let v2 = ys.get(i).and_then(|s| s.parse::<i32>().ok()).unwrap_or(0);
        if v1 > v2 {
            return 1;
        }
        if v1 < v2 {
            return -1;
        }
    }
    0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_compare() {
        assert_eq!(compare_version_string("1.0", "1.1"), -1);
        assert_eq!(compare_version_string("1.10", "1.9"), 1);
        assert_eq!(compare_version_string("1.0.0", "1.0"), 0);
    }
}
