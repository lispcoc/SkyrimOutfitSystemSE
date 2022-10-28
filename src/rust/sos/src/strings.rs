use std::cmp::Ordering;
use natord;

pub fn nat_ord_case_insensitive_c(a: &str, b: &str) -> i8 {
    match natord::compare_ignore_case(a, b) {
        Ordering::Less => 1,
        Ordering::Equal => 0,
        Ordering::Greater => -1,
    }
}