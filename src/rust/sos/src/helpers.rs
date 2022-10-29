#[allow(dead_code)]
pub const fn zero_pad_u8<const N: usize, const M: usize>(arr: &[u8; N]) -> [i8; M] {
    let mut m = [0i8; M];
    let mut i = 0;
    while i < N {
        m[i] = arr[i] as i8;
        i += 1;
    }
    m
}
