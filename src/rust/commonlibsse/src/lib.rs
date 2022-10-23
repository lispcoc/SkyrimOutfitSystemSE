pub use ffi::*;

#[cxx::bridge(namespace = "RE")]
pub mod ffi {
    #[repr(u32)]
    #[derive(Ord, PartialOrd, Eq, PartialEq)]
    pub enum BipedObjectSlot {
        kNone = 0,
        kHead = 1,
        kHair = 2,
        kBody = 4,
        kHands = 8,
        kForearms = 16,
        kAmulet = 32,
        kRing = 64,
        kFeet = 128,
        kCalves = 256,
        kShield = 512,
        kTail = 1024,
        kLongHair = 2048,
        kCirclet = 4096,
        kEars = 8192,
        kModMouth = 16384,
        kModNeck = 32768,
        kModChestPrimary = 65536,
        kModBack = 131072,
        kModMisc1 = 262144,
        kModPelvisPrimary = 524288,
        kDecapitateHead = 1048576,
        kDecapitate = 2097152,
        kModPelvisSecondary = 4194304,
        kModLegRight = 8388608,
        kModLegLeft = 16777216,
        kModFaceJewelry = 33554432,
        kModChestSecondary = 67108864,
        kModShoulder = 134217728,
        kModArmLeft = 268435456,
        kModArmRight = 536870912,
        kModMisc2 = 1073741824,
        kFX01 = 2147483648,
    }

    #[repr(u32)]
    #[derive(Ord, PartialOrd, Eq, PartialEq)]
    pub enum BIPED_OBJECT {
        kHead = 0,
        kHair = 1,
        kBody = 2,
        kHands = 3,
        kForearms = 4,
        kAmulet = 5,
        kRing = 6,
        kFeet = 7,
        kCalves = 8,
        kShield = 9,
        kTail = 10,
        kLongHair = 11,
        kCirclet = 12,
        kEars = 13,
        kModMouth = 14,
        kModNeck = 15,
        kModChestPrimary = 16,
        kModBack = 17,
        kModMisc1 = 18,
        kModPelvisPrimary = 19,
        kDecapitateHead = 20,
        kDecapitate = 21,
        kModPelvisSecondary = 22,
        kModLegRight = 23,
        kModLegLeft = 24,
        kModFaceJewelry = 25,
        kModChestSecondary = 26,
        kModShoulder = 27,
        kModArmLeft = 28,
        kModArmRight = 29,
        kModMisc2 = 30,
        kFX01 = 31,

        kHandToHandMelee = 32,
        kOneHandSword = 33,
        kOneHandDagger = 34,
        kOneHandAxe = 35,
        kOneHandMace = 36,
        kTwoHandMelee = 37,
        kBow = 38,
        kStaff = 39,
        kCrossbow = 40,
        kQuiver = 41,

        kTotal = 42
    }
    unsafe extern "C++" {
        include!("commonlibsse/include/customize.h");
        pub type TESObjectARMO;
        pub fn GetSlotMask(self: &TESObjectARMO) -> BipedObjectSlot;
        pub fn GetFormID(self: &TESObjectARMO) -> u32;
        pub type BipedObjectSlot;
        pub type BIPED_OBJECT;
        pub type PlayerCharacter;
        pub fn PlayerCharacter_GetSingleton() -> *mut PlayerCharacter;
    }
}

impl BIPED_OBJECT {
    pub const MAX_IN_GAME: usize = BIPED_OBJECT::kHandToHandMelee.repr as usize;
}

impl TESObjectARMO {
    pub fn assign_using_mask(&mut self, dest: &mut [*mut TESObjectARMO; BIPED_OBJECT::MAX_IN_GAME]) {
        let mask = self.GetSlotMask().repr;
        for slot in 0..BIPED_OBJECT::MAX_IN_GAME {
            if mask & (1 << slot) != 0 {
                dest[slot] = self;
            }
        }
    }
}