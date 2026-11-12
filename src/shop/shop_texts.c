// The shop's and the underground market's texts, which the desktop's
// menus draw (___26650h.c, ___269e4h.c, ___2d054h.c, ___2d0ech.c,
// ___2d294h.c, ___26fach.c) and the GBA's show: what each upgrade and
// item does, once on show and once bought, what the repairs cost you in
// words, and the loan shark's offers. The engines on show are each car's
// own, in car_data.c's txt_engine_upgrades.

#include "drally.h"
#include "drally_shop.h"

// the engine on its fitting, by the level it was
const char40 ___191100h[4][6] = {
	[0] = {
		[0] = "[4 cylinder motor installed",
		[1] = "",
		[2] = "This engine genie will let you",
		[3] = "win. It will grant your boy racer",
		[4] = "wishes.",
		[5] = ""
	},
	[1] = {
		[0] = "[Enhanced 4 cylinder installed",
		[1] = "",
		[2] = "Now any bitter regrets are in the",
		[3] = "past, just blurred, sweet street-",
		[4] = "lights speeding past fast.",
		[5] = ""
	},
	[2] = {
		[0] = "[V6 Injection motor installed",
		[1] = "",
		[2] = "If speeding is the 8th deadly sin,",
		[3] = "with this babe under your hood,",
		[4] = "you are now past redemption.",
		[5] = ""
	},
	[3] = {
		[0] = "[V8 Turbo Injection",
		[1] = "",
		[2] = "Gleaming pistons pumping in chorus",
		[3] = "with your hungry heart. This sweet",
		[4] = "symbiosis can cause addiction.",
		[5] = ""
	}
};

// the tires on buying them
const char40 ___191880h[4][6] = {
	[0] = {
		[0] = "[Trackmaster 155 bought",
		[1] = "",
		[2] = "Warp 9, Captain. No problems with",
		[3] = "traction now. The finish line,",
		[4] = "she's closing fast.",
		[5] = ""
	},
	[1] = {
		[0] = "[Roadrunner NS 175 bought",
		[1] = "",
		[2] = "On these wheels, your vehicle",
		[3] = "starts like a nuclear blast, and",
		[4] = "stops you like a cardiac arrest.",
		[5] = ""
	},
	[2] = {
		[0] = "[Slidestop 200 bought",
		[1] = "",
		[2] = "Now fast talk to that asphalt,",
		[3] = "motormouth. Time to make some",
		[4] = "beautiful automobile poetry.",
		[5] = ""
	},
	[3] = {
		[0] = "[Racer-X230 bought",
		[1] = "",
		[2] = "Smart move, motorman. Now get out",
		[3] = "there, and do some dashing, dare-",
		[4] = "devilish damage.",
		[5] = ""
	}
};

// the armor on buying it
const char40 ___192000h[4][6] = {
	[0] = {
		[0] = "[Steel Triumph bought",
		[1] = "",
		[2] = "Straight from the best R&D Labs,",
		[3] = "shielding Steel Triumph gives you",
		[4] = "that extra edge you crave.",
		[5] = ""
	},
	[1] = {
		[0] = "[FerroChromium Armor",
		[1] = "",
		[2] = "Deflects bullets, and most other",
		[3] = "bad stuff that goes bang in the",
		[4] = "night, too.",
		[5] = ""
	},
	[2] = {
		[0] = "[Titanium Plates installed",
		[1] = "",
		[2] = "Shiny hard stuff. No explosive can",
		[3] = "shadow its tanning radiance. Don't",
		[4] = "forget your shades!",
		[5] = ""
	},
	[3] = {
		[0] = "[Accesteel Armor installed",
		[1] = "",
		[2] = "Not even a shell-shock serious",
		[3] = "bazooka can faze the cool mirror",
		[4] = "beauty of Accesteel.",
		[5] = ""
	}
};

// the tires on show
const char40 ___1914c0h[4][6] = {
	[0] = {
		[0] = "[Trackmaster 155",
		[1] = "",
		[2] = "These wheels will let you do some",
		[3] = "rapid fire cornerings on that",
		[4] = "shimmering silver street.",
		[5] = ""
	},
	[1] = {
		[0] = "[Roadrunner NS 175",
		[1] = "",
		[2] = "With these fat tires, you'll shoot",
		[3] = "from the starting-line like a",
		[4] = "blood lusting bullet.",
		[5] = ""
	},
	[2] = {
		[0] = "[Slidestop 200",
		[1] = "",
		[2] = "These fat bottomed tires'll kiss",
		[3] = "the road like crazy, they just",
		[4] = "can't get enough. They're hot.",
		[5] = ""
	},
	[3] = {
		[0] = "[Racer-X230",
		[1] = "",
		[2] = "Zorro's Zs were nothing. Carve",
		[3] = "your whole signature, to the",
		[4] = "rainbow-oiled road, doing 160.",
		[5] = ""
	}
};

// the armor on show
const char40 ___191c40h[4][6] = {
	[0] = {
		[0] = "[Steel Triumph Plating",
		[1] = "",
		[2] = "Steel Triumph won't stop the hurt,",
		[3] = "but it just might keep you in one",
		[4] = "piece to the finish line.",
		[5] = ""
	},
	[1] = {
		[0] = "[FerroChromium Armor",
		[1] = "",
		[2] = "What the rival guns give, this",
		[3] = "armor can take, but you'd better",
		[4] = "shoot back, there's a limit.",
		[5] = ""
	},
	[2] = {
		[0] = "[Titanium Plating",
		[1] = "",
		[2] = "See this big gun; see this armor.",
		[3] = "[BANG!{ Now see the armor. Smile to",
		[4] = "your reflection. Looking good.",
		[5] = ""
	},
	[3] = {
		[0] = "[Accesteel Armor",
		[1] = "",
		[2] = "All but the most extreme shots",
		[3] = "bounce off like hail on hard",
		[4] = "concrete.",
		[5] = ""
	}
};

// the most the car takes
const char40 MaxUpgrades[3][6] = {
	[0] = {
		[0] = "[NO MORE ENGINE UPGRADES",
		[1] = "",
		[2] = "You've got the most beastly motor",
		[3] = "monster this vehicle can carry.",
		[4] = "Hear its powerful & sublime sound.",
		[5] = ""
	},
	[1] = {
		[0] = "[NO MORE TIRE UPGRADES",
		[1] = "",
		[2] = "You've got the best fat, sticky",
		[3] = "tires this vehicle can boast. You",
		[4] = "are in close touch with the road.",
		[5] = ""
	},
	[2] = {
		[0] = "[NO MORE ARMOR UPGRADES",
		[1] = "",
		[2] = "You got the best shining knight's",
		[3] = "armor this warhorsepower chariot",
		[4] = "can carry to any road battle.",
		[5] = ""
	}
};

// a repair, by how much damage: 100%, 90-99%, ..., 1-9%, none
const char40 ___194170h[12][6] = {
	[0] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "This used to be a car, huh? Carry",
		[3] = "the rest of it in here & we'll see",
		[4] = "what can be done.",
		[5] = ""
	},
	[1] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Sorry, this vehicle is gone for",
		[3] = "good... Just kidding. Medic, fast!",
		[4] = "We've got a driver in shock.",
		[5] = ""
	},
	[2] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Don't tell me. You parked your car",
		[3] = "in a nuclear test zone? These",
		[4] = "holes need some gum, start chewing.",
		[5] = ""
	},
	[3] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Who helped you tow this piece of",
		[3] = "junk over the finish line? No",
		[4] = "sweat, this can be fixed.",
		[5] = ""
	},
	[4] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "So. Your car's sorry ass got blown",
		[3] = "clean off. Acme glue will fix your",
		[4] = "mishap-ridden day at the races.",
		[5] = ""
	},
	[5] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "As Mr. Nukem sometimes puts it:",
		[3] = "'Uh-oh, that's gotta hurt.' This",
		[4] = "calls for a screwdriver & aspirin.",
		[5] = ""
	},
	[6] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Your conveyance's methylated",
		[3] = "spirit receptacle has developed a",
		[4] = "chronic hernia, or something...",
		[5] = ""
	},
	[7] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "This is no time to rest in pieces.",
		[3] = "The race awaits! She'll be back on",
		[4] = "her wheels in no time.",
		[5] = ""
	},
	[8] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Take five, driver. A straight &",
		[3] = "simple sledgehammer job here.",
		[4] = "These percussions, gotta love 'em.",
		[5] = ""
	},
	[9] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Nothing a fresh paint-job can't",
		[3] = "fix. You'll be back on the track",
		[4] = "in a pit-stop minute.",
		[5] = ""
	},
	[10] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Got your wax coat scratched? Want",
		[3] = "to be your shining best? Hey, what-",
		[4] = "ever lights up your dashboard.",
		[5] = ""
	},
	[11] = {
		[0] = "[Repair",
		[1] = "",
		[2] = "Perfect is perfect. What ain't",
		[3] = "broken, can't be fixed. Come back",
		[4] = "after you've taken in some.",
		[5] = ""
	}
};

// the market's items on show: mines, spiked bumpers, rocket fuel, sabotage
const char40 ___193720h[4][6] = {
	[0] = {
		[0] = "[Mines",
		[1] = "",
		[2] = "Sow these seeds of destruction",
		[3] = "after a blind bend where headlight",
		[4] = "eyes are tire-smoke tired.",
		[5] = ""
	},
	[1] = {
		[0] = "[Spiked Bumpers",
		[1] = "",
		[2] = "Nothing wakes you up like an enemy",
		[3] = "car, closing in to give you",
		[4] = "a sharp kiss with spiked bumpers.",
		[5] = ""
	},
	[2] = {
		[0] = "[Rocket Fuel",
		[1] = "",
		[2] = "This nectar in the veins of your",
		[3] = "vehicle takes you right into",
		[4] = "orbit, real speedy like.",
		[5] = ""
	},
	[3] = {
		[0] = "[Sabotage",
		[1] = "",
		[2] = "Do you have bad luck always riding",
		[3] = "shotgun? Time for that golden",
		[4] = "racer boy to get some. Right?",
		[5] = ""
	}
};

// the market's items bought
const char40 ___193db0h[4][6] = {
	[0] = {
		[0] = "[8 Mines loaded into your car",
		[1] = "",
		[2] = "Now those rival wheels will reap",
		[3] = "only burning engine death, and",
		[4] = "that road will be all yours.",
		[5] = ""
	},
	[1] = {
		[0] = "[Spiked bumpers installed",
		[1] = "",
		[2] = "Now you are saying: 'Stay back or",
		[3] = "you sorry, you dead.' Now that's",
		[4] = "the attitude.",
		[5] = ""
	},
	[2] = {
		[0] = "[Rocket Fuel bought",
		[1] = "",
		[2] = "Warning! We're talking 'bout a",
		[3] = "backdraft barbecue party here.",
		[4] = "[Using the fuel causes damage!{",
		[5] = ""
	},
	[3] = {
		[0] = "[Sabotage in progress",
		[1] = "",
		[2] = "Sweet deal. Now your arch-racing-",
		[3] = "rival will tailslide down that",
		[4] = "vile road to wreckhood oblivion.",
		[5] = ""
	}
};

// the loan shark's offer, by 5 less your car: the Deliverator's first
const char40 ___1923c0h[5][6] = {
	[0] = {
		[0] = "[Borrow $12,000 - Pay $18,000",
		[1] = "",
		[2] = "With better gear you'd already be",
		[3] = "on the top, eh? So how does a",
		[4] = "short-term loan of [$12,000{ sound?",
		[5] = ""
	},
	[1] = {
		[0] = "[Borrow $9,000 - Pay $13,500",
		[1] = "",
		[2] = "With better gear you'd already be",
		[3] = "on the top, eh? So how does a",
		[4] = "short-term loan of [$9,000{ sound?",
		[5] = ""
	},
	[2] = {
		[0] = "[Borrow $6,000 - Pay $9,000",
		[1] = "",
		[2] = "With better gear you'd already be",
		[3] = "on the top, eh? So how does a",
		[4] = "short-term loan of [$6,000{ sound?",
		[5] = ""
	},
	[3] = {
		[0] = "[Borrow $3,000 - Pay $4,500",
		[1] = "",
		[2] = "With better gear you'd already be",
		[3] = "on the top, eh? So how does a",
		[4] = "short-term loan of [$3,000{ sound?",
		[5] = ""
	},
	[4] = {
		[0] = "[Borrow $1,500 - Pay $2,250",
		[1] = "",
		[2] = "With better gear you'd already be",
		[3] = "on the top, eh? So how does a",
		[4] = "short-term loan of [$1,500{ sound?",
		[5] = ""
	}
};

// the loan granted, the same way
const char40 ___192870h[5][6] = {
	[0] = {
		[0] = "[Loan of $12,000 granted.",
		[1] = "",
		[2] = "You have 3 races to pay me back the",
		[3] = "[$18,000{. Fair is fair [<GRIN>{.",
		[4] = "",
		[5] = "Press [ESC{ to go back to the shop."
	},
	[1] = {
		[0] = "[Loan of $9,000 granted.",
		[1] = "",
		[2] = "You have 3 races to pay me back the",
		[3] = "[$13,500{. Fair is fair [<GRIN>{.",
		[4] = "",
		[5] = "Press [ESC{ to go back to the shop."
	},
	[2] = {
		[0] = "[Loan of $6,000 granted.",
		[1] = "",
		[2] = "You have 3 races to pay me back the",
		[3] = "[$9,000{. Fair is fair [<GRIN>{.",
		[4] = "",
		[5] = "Press [ESC{ to go back to the shop."
	},
	[3] = {
		[0] = "[Loan of $3,000 granted.",
		[1] = "",
		[2] = "You have 3 races to pay me back the",
		[3] = "[$4,500{. Fair is fair [<GRIN>{.",
		[4] = "",
		[5] = "Press [ESC{ to go back to the shop."
	},
	[4] = {
		[0] = "[Loan of $1,500 granted.",
		[1] = "",
		[2] = "You have 3 races to pay me back the",
		[3] = "[$2,250{. Fair is fair [<GRIN>{.",
		[4] = "",
		[5] = "Press [ESC{ to go back to the shop."
	}
};
