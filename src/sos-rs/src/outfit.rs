use std::collections::HashSet;
use crate::Armor;

pub struct Outfit {
    pub name: String,
    pub armors: HashSet<*mut Armor>,
}
