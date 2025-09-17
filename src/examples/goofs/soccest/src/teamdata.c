//teamdata.C
//Generates team data

#include "teamdata.h"
#include "statfunc.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

const char *names_forename[] = 
{
	"Aaron", "Adam", "Adrian", "Alan", "Alana", "Albert", "Alex", "Alexander", "Alexandra", "Ali", "Alice", 
	"Alicia", "Alison", "Alistair", "Allan", "Allen", "Allison", "Amanda", "Amber", "Amy", "Andre", "Andrea",
	"Andrew", "Angela", "Angie", "Anita", "Ann", "Anna", "Anne", "Annette", "Annie", "Anthony", "Anton", 
	"Antonio", "Arthur", "Ashleigh", "Ashley", "Audrey", "Barbara", "Barry", "Belinda", "Benjamin", "Bernadette",
	"Bernard", "Bernie", "Beth", "Betty", "Bev", "Beverley", "Bianca", "Bill", "Blake", "Bob", "Boer", "Brad",
	"Bradley", "Brenda", "Brendan", "Brendon", "Brent", "Brett", "Brian", "Bridget", "Bronwyn", "Brooke", "Bruce",
	"Bryan", "Caitlin", "Cameron", "Carl", "Carly", "Carmel", "Carol", "Carole", "Caroline", "Carolyn", "Casey",
	"Cassandra", "Catherine", "Cathy", "Chad", "Charles", "Charlie", "Charlotte", "Charmaine", "Cherie", "Cheryl",
	"Chloe", "Chris", "Christian", "Christina", "Christine", "Christopher", "Cindy", "Claire", "Clare", "Clayton",
	"Cliff", "Clint", "Clinton", "Clive", "Colin", "Colleen", "Corey", "Courtney", "Craig", "Cynthia", "Dale", 
	"Damian", "Damien", "Dan", "Daniel", "Danielle", "Danny", "Darren", "Darryl", "Daryl", "David", "Dawn",
	"Dean", "Deb", "Debbie", "Deborah", "Debra", "Dee", "Denis", "Denise", "Dennis", "Derek", "Des", "Diana", 
	"Diane", "Dianne", "Dominic", "Don", "Donald", "Donna", "Donne", "Dorothy", "Doug", "Douglas", "Drew", 
	"Duncan", "Dylan", "Eddie", "Edward", "Eileen", "Elaine", "Elizabeth", "Emily", "Emma", "Eric", "Erica", 
	"Erin", "Eugene", "Eva", "Evan", "Felicity", "Fiona", "Fran", "Frances", "Francis", "Frank", "Fred", "Gail", 
	"Gareth", "Garry", "Gary", "Gavin", "Gemma", "Geoff", "Geoffrey", "George", "Georgia", "Georgina", 
	"Geraldine", "Gerard", "Gerry", "Gillian", "Gina", "Glen", "Glenda", "Glenn", "Gordon", "Grace", "Graeme", 
	"Graham", "Grant", "Greg", "Gregory", "Guy", "Hannah", "Hans", "Harry", "Hayden", "Hayley", "Hazel", 
	"Heather", "Heidi", "Helen", "Henry", "Holly", "Hugh", "Iain", "Ian", "Ingrid", "Irene", "Ivan", "Jack", 
	"Jackie", "Jacob", "Jacqueline", "Jacqui", "Jade", "Jake", "James", "Jamie", "Jan", "Jane", "Janelle",
	"Janet", "Janice", "Janine", "Jared", "Jarrad", "Jasmine", "Jason", "Jay", "Jayne", "Jean", "Jeanette", 
	"Jeff", "Jeffrey", "Jen", "Jenni", "Jennifer", "Jenny", "Jeremy", "Jess", "Jesse", "Jessica", "Jessie", 
	"Jill", "Jim", "Jo", "Joan", "Joanna", "Joanne", "Jodi", "Jodie", "Jody", "Joe", "Joel", "John", "Jon", 
	"Jonathan", "Jones", "Jordan", "Joseph", "Josephine", "Josh", "Joshua", "Joy", "Joyce", "Judith", "Judy",
	"Julia", "Julian", "Julie", "June", "Justin", "Justine", "Karen", "Karl", "Kate", "Katherine", "Kathleen",
	"Kathryn", "Kathy", "Katie", "Katrina", "Kay", "Kaye", "Keith", "Kellie", "Kelly", "Kelvin", "Ken", "Kenneth",
	"Kerry", "Kevin", "Kieran", "Kim", "Kimberley", "Kirsten", "Kirsty", "Kris", "Kristy", "Kyle", "Kylie", "Kym", 
	"Lance", "Lara", "Larry", "Laura", "Lauren", "Laurie", "Lawrence", "Leah", "Leanne", "Leao", "Lee", "Leigh", 
	"Len", "Leon", "Leonie", "Les", "Lesley", "Leslie", "Liam", "Libby", "Linda", "Lindsay", "Lisa", "Lloyd",
	"Lorna", "Lorraine", "Louis", "Louise", "Lucy", "Luke", "Lyn", "Lynda", "Lynette", "Lynn", "Lynne", "Maggie",
	"Mal", "Malcolm", "Mandy", "Marc", "Marcus", "Maree", "Margaret", "Maria", "Marie", "Marilyn", "Marina", 
	"Mario", "Marion", "Mark", "Martin", "Mary", "Mathew", "Matt", "Matthew", "Maureen", "Maurice", "Max", 
	"Megan", "Mel", "Melanie", "Melinda", "Melissa", "Michael", "Michele", "Michelle", "Mick", "Mitch", 
	"Mitchell", "Mohammad", "Monica", "Monique", "Murray", "Nadia", "Nadine", "Naomi", "Narelle", "Natalie", 
	"Natasha", "Nathan", "Neil", "Neville", "Nic", "Nicholas", "Nick", "Nicky", "Nicola", "Nicole", "Nigel", 
	"Nikki", "Nina", "Noel", "Norman", "Oliver", "Olivia", "Owen", "Pam", "Pamela", "Pat", "Patrica", "Patricia",
	"Patrick", "Paul", "Paula", "Pauline", "Penny", "Peta", "Peter", "Phil", "Philip", "Phillip", "Rachael", 
	"Rachel", "Ralph", "Ray", "Raymond", "Rebecca", "Renae", "Renee", "Rex", "Rhonda", "Rhys", "Ric", "Richard",
	"Rick", "Ricky", "Rita", "Robert", "Robin", "Robyn", "Rod", "Rodney", "Roger", "Ron", "Ronald", "Rory", 
	"Rosa", "Rose", "Rosemary", "Ross", "Roy", "Russell", "Ruth", "Ryan", "Sally", "Sam", "Samantha", "Samuel",
	"Sandra", "Sandy", "Sara", "Sarah", "Scott", "Sean", "Shane", "Shannon", "Sharon", "Shaun", "Shelley", 
	"Shirley", "Simon", "Simone", "Smith", "Sonia", "Sophie", "Stacey", "Stefan", "Stephanie", "Stephen", 
	"Steven", "Stewart", "Stuart", "Sue", "Susan", "Suzanne", "Sylvia", "Tamara", "Tammy", "Tania", "Tanya", 
	"Tara", "Ted", "Teresa", "Terri", "Terry", "Thomas", "Tim", "Timothy", "Tina", "Todd", "Toni", "Tony", 
	"Tracey", "Tracy", "Travis", "Trent", "Trevor", "Trish", "Tristan", "Troy", "Val", "Valerie", "Vanessa", 
	"Veronica", "Vicki", "Vicky", "Victor", "Victoria", "Vince", "Vincent", "Wade", "Walter", "Warren", "Wayne",
	"Wendy", "Will", "William", "Yvonne", "Zoe", 
};


const char *names_surname[] = 
{
	"Abbott", "Adams", "Aitken", "Alexander", "Allan", "Allen", "Anderson", "Andrew", "Andrews", "Anthony", 
	"Archer", "Armstrong", "Arnold", "Ashworth", "Atkins", "Atkinson", "Austin", "Bailey", "Baker", "Ball", 
	"Banks", "Barber", "Barker", "Barnes", "Barnett", "Barr", "Barrett", "Barry", "Bartlett", "Barton", "Bates",
	"Baxter", "Beard", "Bell", "Bennett", "Benson", "Bentley", "Berry", "Best", "Birch", "Bird", "Bishop",
	"Black", "Blair", "Blake", "Bolton", "Bond", "Booth", "Bowden", "Bowen", "Bowman", "Boyd", "Boyle", "Bradley",
	"Bradshaw", "Brady", "Bray", "Brennan", "Briggs", "Brooks", "Brown", "Browne", "Bruce", "Bryant", "Buchanan",
	"Buckley", "Bull", "Burgess", "Burke", "Burnett", "Burns", "Burrows", "Burton", "Butcher", "Butler", "Byrne",
	"Cahill", "Cameron", "Campbell", "Carey", "Carr", "Carroll", "Carter", "Casey", "Chambers", "Chan", 
	"Chandler", "Chapman", "Chen", "Chin", "Chong", "Christie", "Clark", "Clarke", "Clayton", "Clements",
	"Clifford", "Clifton", "Cole", "Coleman", "Coles", "Collins", "Connolly", "Connor", "Cook", "Cooke",
	"Cooper", "Cox", "Craig", "Crane", "Crawford", "Cross", "Cullen", "Cunningham", "Currie", "Curtis",
	"Dale", "Daly", "Daniel", "Daniels", "Davey", "Davidson", "Davies", "Davis", "Dawson", "Day", "Dean", 
	"Dennis", "Dickson", "Dillon", "Dixon", "Dodd", "Doherty", "Donaldson", "Donnelly", "Donovan", "Douglas", 
	"Dowling", "Doyle", "Duffy", "Duncan", "Dunn", "Dwyer", "Dyer", "Dyson", "Eaton", "Edmonds", "Edwards", 
	"Egan", "Elliott", "Ellis", "Emery", "English", "Evans", "Farmer", "Farrell", "Faulkner", "Ferguson", "Field",
	"Fisher", "FitzGerald", "FitzPatrick", "Fleay", "Fleming", "Fletcher", "Flynn", "Foley", "Forbes", "Ford", 
	"Forrest", "Foster", "Fowler", "Fox", "Francis", "Franklin", "Fraser", "Freeman", "French", "Frost", "Fry",
	"Fuller", "Gale", "Gallagher", "Gardiner", "Gardner", "George", "Gibbs", "Gibson", "Gilbert", "Giles", "Gill",
	"Gillespie", "Goddard", "Godfrey", "Goh", "Goodwin", "Gordon", "Gould", "Graham", "Grant", "Gray", "Green", 
	"Gregory", "Griffin", "Griffiths", "Hall", "Hamilton", "Hammond", "Hancock", "Hansen", "Hanson", "Harding",
	"Hardy", "Harper", "Harris", "Harrison", "Hart", "Hartley", "Harvey", "Harwood", "Hawkins", "Hay", "Hayes",
	"Haynes", "Hayward", "Healy", "Heath", "Henderson", "Henry", "Herbert", "Hewitt", "Hicks", "Higgins", "Hill",
	"Hills", "Ho", "Hobbs", "Hodges", "Hodgson", "Hogan", "Holland", "Holmes", "Holt", "Hooper", "Hopkins", 
	"Horton", "Howard", "Howe", "Howell", "Hudson", "Hughes", "Hunt", "Hunter", "Hutchinson", "Hutton", "Hyde",
	"Ingram", "Ireland", "Italiano", "Jackson", "Jacobs", "James", "Jamieson", "Jarvis", "Jeffery", "Jenkins",
	"Jennings", "Jensen", "John", "Johns", "Johnson", "Johnston", "Johnstone", "Jones", "Jordan", "Joyce", 
	"Kay", "Keenan", "Kelly", "Kemp", "Kennedy", "Kenny", "Kent", "Kerr", "Kim", "King", "Kirk", "Knight", 
	"Kumar", "Lam", "Lamb", "Lambert", "Lane", "Lang", "Law", "Lawrence", "Lawson", "Le", "Leach", "Lee", 
	"Lewis", "Li", "Lim", "Lin", "Lindsay", "Little", "Liu", "Lloyd", "Logan", "Long", "Love", "Low", "Lowe", 
	"Lucas", "Lynch", "Lyons", "MacDonald", "MacKay", "MacKenzie", "Maher", "Mann", "Manning", "Marsh", 
	"Marshall", "Martin", "Mason", "Masters", "Matthews", "Maxwell", "May", "McCarthy", "McDonald", "McGrath",
	"McGregor", "McKay", "McKenna", "McKenzie", "McLean", "McLeod", "McMahon", "McNamara", "Middleton", "Miles",
	"Millar", "Miller", "Mills", "Milne", "Mitchell", "Moore", "Morgan", "Morris", "Morrison", "Morton", "Moss",
	"Muir", "Munro", "Murphy", "Murray", "Nash", "Nelson", "Newman", "Newton", "Ng", "Nguyen", "Nicholas", 
	"Nicholls", "Nichols", "Nicholson", "Noble", "Norman", "Norris", "O'Brien", "O'Connor", "O'Donnell", 
	"O'Neill", "O'Sullivan", "Oliver", "Ong", "Osborne", "Owen", "Page", "Palmer", "Park", "Parker", "Parry",
	"Parsons", "Pascoe", "Patel", "Paterson", "Patterson", "Paul", "Payne", "Pearce", "Pearson", "Pereira", 
	"Perkins", "Perry", "Peters", "Phillips", "Pike", "Pollard", "Poole", "Porter", "Potter", "Powell", "Power",
	"Pratt", "Price", "Pritchard", "Quinn", "Read", "Reed", "Rees", "Reeves", "Reid", "Reynolds", "Rhodes", 
	"Rice", "Richards", "Richardson", "Riley", "Ritchie", "Roberts", "Robertson", "Robinson", "Robson", 
	"Rodgers", "Rogers", "Rose", "Ross", "Rowe", "Russell", "Ryan", "Sanders", "Saunders", "Savage", "Scott",
	"Shah", "Sharp", "Sharpe", "Shaw", "Shepherd", "Simmons", "Simpson", "Sims", "Sinclair", "Singh", "Skinner",
	"Slater", "Smart", "Smith", "Spencer", "Stanley", "Steele", "Stephens", "Stevens", "Stevenson", "Stewart", 
	"Stokes", "Stone", "Stuart", "Sullivan", "Sutherland", "Sutton", "Tan", "Taylor", "Thomas", "Thompson", 
	"Thomson", "Thornton", "Todd", "Townsend", "Tran", "Tucker", "Turner", "Vincent", "Walker", "Wall", 
	"Wallace", "Wallis", "Walsh", "Walters", "Walton", "Wang", "Ward", "Warner", "Warren", "Waters", 
	"Watkins", "Watson", "Watt", "Watts", "Webb", "Webster", "Weir", "Wells", "West", "Weston", "Wheeler",
	"White", "Whyte", "Wilkins", "Wilkinson", "Williams", "Williamson", "Willis", "Wilson", "Winter", "Wong",
	"Wood", "Woods", "Woodward", "Wright", "Yates", "Young", "Zhang"
};

const char *names_mammal[] = 
{
	"Antechinuses", "Bandicoots", "Bantengs", "Bats", "Bentwingbats", "Bettongs", "Bilbys", "Bindjulangs",
	"Blackbucks", "Blossombats", "Brumbys", "Buffaloes", "Camels", "Cats", "Cattles", "Chitals", "Chuditches", 
	"Cuscuses", "Deer", "Devils", "Dibblers", "Diguls", "Dingoes", "Djoongaris", "Dogs", "Dolphins", "Donkeys", 
	"Dromedarys", "Dugongs", "Dunnarts", "Echidnas", "Falsistrelles", "Fieldrats", "Flyingfoxes", "Foxes", 
	"Freetailbats", "Fruitbats", "Furseals", "Gliders", "Goats", "Hares", "Harewallabys", "Hoppingmice", "Horses", 
	"Horseshoebats", "Kalunyjas", "Kalutas", "Kangaroos", "Koalas", "Konooms", "Koomals", "Kowaris", "Kultarrs",
	"Leafnosedbats", "Luaners", "Malas", "Mastiffbats", "Melomyss", "Mice", "Moles", "Mulgaras", "Myotiss", 
	"Nabarleks", "Ningauis", "Numbats", "Pademelons", "Phalangers", "Phascogales", "Pigs", "Pipistrelles", 
	"Planigales", "Platypuses", "Porpoises", "Possums", "Potoroos", "Pseudantechinuses", "Pygmypossums",
	"Quokkas", "Quolls", "Rabbitrats", "Rabbits", "Ratkangaroos", "Rats", "Rockrats", "Rockwallabys", 
	"Sambars", "Sealions", "Seals", "Sheathtailbats", "Shrews", "Thylacines", "Treekangaroos", "Treerats",
	"Uromyss", "Wallabys", "Wallaroos", "Warabis", "Warrus", "Waterrats", "Whales", "Wilijis", "Wombats",	
};

const char *names_city[] = 
{
	"Abba River", "Abbey", "Acton Park", "Ajana", "Albany", "Aldersyde", "Alexander Heights", "Alexandra Bridge",
	"Alfred Cove", "Alkimos", "Allanooka", "Allanson", "Ambergate", "Amelup", "Anketell", "Anniebrook",
	"Applecross", "Applecross North", "Ardath", "Ardross", "Argyle", "Armadale", "Arrino", "Arrowsmith", 
	"Arthur River", "Ascot", "Ashby", "Ashfield", "Attadale", "Atwell", "Aubin Grove", "Augusta", "Australind",
	"Aveley", "Baandee", "Babakin", "Babbage Island", "Badgebup", "Badgerin Rock", "Badgin", "Badgingarra",
	"Bakers Hill", "Balbarrup", "Balcatta", "Baldivis", "Balga", "Balingup", "Balkuling", "Balladonia", 
	"Ballajura", "Ballaying", "Ballidu", "Bally Bally", "Bandy Creek", "Banjup", "Banksia Grove", 
	"Barragup", "Baskerville", "Bassendean", "Bateman", "Bayonet Head", "Bayswater", "Beachlands", 
	"Beacon", "Beaconsfield", "Beaufort River", "Beaumont", "Beckenham", "Bedford", "Bedfordale", 
	"Beechboro", "Beechina", "Beedelup", "Beela", "Beelerup", "Beeliar", "Beenong", "Beermullah",
	"Bejoording", "Beldon", "Belhus", "Bellevue", "Belmont", "Bencubbin", "Benger", "Benjaberring", "Benjinup",
	"Bennett Springs", "Bentley", "Beresford", "Bertram", "Beverley", "Bibra Lake", "Bickley", "Bicton", 
	"Big Grove", "Bilbarin", "Bilingurr", "Bindi Bindi", "Bindoon", "Binningup", "Bluff Point", "Blythewood", 
	"Boallia", "Bodallin", "Boddington", "Bokal", "Bolgart", "Bonnie Rock", "Boodarie", "Booragoon", "Borden", 
	"Bornholm", "Boscabel", "Boulder", "Boundain", "Bouvard", "Bovell", "Bow Bridge", "Bowelling", "Boya", 
	"Boyanup", "Boyatup", "Boyerine", "Boyup Brook", "Bramley", "Bremer Bay", "Brentwood", "Bridgetown", 
	"Brigadoon", "Broadwater", "Broadway Nedlands", "Broadwood", "Brockman", "Brookdale", "Brookhampton",
	"Brookton", "Broome", "Broomehill", "Broomehill East", "Brown Range", "Bruce Rock", "Brunswick", "Buckingham",
	"Buckland", "Bulgarra", "Bull Creek", "Bullaring", "Bullfinch", "Bullock Hills", "Bullsbrook", "Bunbury", 
	"Buntine", "Burakin", "Burekup", "Burlong", "Burns Beach", "Burnside", "Burracoppin", "Burswood", "Busselton",
	"Butler", "Byford", "Cadoux", "Caiguna", "Calingiri", "Calista", "Camballin", "Camillo", 
	"Canning Bridge Applecross", "Canning Mills", "Canning Vale", "Canning Vale East", "Canning Vale South", 
	"Cannington", "Cape Range National Park", "Capel", "Capel River", "Carabooda", "Carani", "Carbunup River", 
	"Cardiff", "Cardup", "Carey Park", "Carine", "Carlisle", "Carlisle North", "Carlisle South", "Carmel", 
	"Carnamah", "Carnarvon", "Carramar", "Carrolup", "Cartmeticup", "Cascade", "Castletown", "Casuarina",
	"Cataby", "Catterick", "Caversham", "Centennial Park", "Cervantes", "Chadwick", "Champion Lakes", 
	"Chapman Hill", "Chidlow", "Chittering", "Churchlands", "City Beach", "Clackline", "Claremont", 
	"Claremont North", "Clarkson", "Cloverdale", "Coblinine", "Cockburn Central", "Collanilling", "College Grove",
	"Collie", "Collie Burn", "Collingwood Heights", "Collingwood Park", "Como", "Condingup", "Connolly", 
	"Contine", "Coodanup", "Coogee", "Cookernup", "Coolbellup", "Coolbinia", "Coolgardie", "Cooloongup", 
	"Coolup", "Coomalbidgup", "Coomberdale", "Coondle", "Coorow", "Coral Bay", "Corrigin", "Cottesloe", 
	"Cowaramup", "Cowcowing", "Coyrecup", "Craigie", "Cranbrook", "Crawley", "Crowea", "Cuballing", "Cue",
	"Culham", "Cullacabardee", "Cunderdin", "Cundinup", "Cunjardine", "Currambine", "Cuthbert", "Daglish",
	"Dalkeith", "Dalwallinu", "Dalyellup", "Dalyup", "Dampier", "Dampier Peninsula", "Dandaragan", "Dangin", 
	"Darch", "Dardadine", "Dardanup", "Dardanup West", "Darkan", "Darling Downs", "Darlington", "Datatine", 
	"Davenport", "Dawesville", "Deanmill", "Denbarker", "Denham", "Denmark", "Derby", "Diamond Tree", "Dianella",
	"Dingup", "Dinninup", "Dixvale", "Djugun", "Dongara", "Dongolocking", "Donnelly River", "Donnybrook", 
	"Doodlakine", "Doubleview", "Dowerin", "Drome", "Drummond Cove", "Dryandra", "Dudinin", "Dudley Park",
	"Dukin", "Dulyalbin", "Dumbarton", "Dumberning", "Dumbleyung", "Duncraig", "Dundas", "Dunsborough", 
	"Duranillin", "Dwellingup", "Eagle Bay", "East Bunbury", "East Cannington", "East Carnarvon",
	"East Fremantle", "East Munglinup", "East Perth", "East Rockingham", "East Victoria Park", "Eaton",
	"Eden Hill", "Edgewater", "Eglinton", "Eighty Mile Beach", "Elgin", "Elleker", "Ellenbrook", "Embleton",
	"Emu Point", "Eneabba", "Eradu", "Erskine", "Esperance", "Eucla", "Ewlyamartup", "Exmouth", "Exmouth Gulf", 
	"Fairbridge", "Falcon", "Ferguson", "Ferndale", "Fitzgerald", "Fitzroy Crossing", "Floreat", "Forest Grove",
	"Forrest", "Forrest Beach", "Forrestdale", "Forrestfield", "Fraser Range", "Fremantle", "Frenchman Bay", 
	"Furnissdale", "Gabbin", "Gairdner", "Gelorup", "Geographe", "Geraldton", "Ghooli", "Gibson", "Gillingarra", 
	"Gingin", "Girrawheen", "Gledhow", "Glen Forrest", "Glen Iris", "Glendalough", "Glenfield", "Glenoran", 
	"Gnangara", "Gnarabup", "Gnowangerup", "Gnowellen", "Golden Bay", "Goode Beach", "Goodlands", "Goomalling",
	"Goomarin", "Gooseberry Hill", "Gorge Rock", "Gorrie", "Gosnells", "Gracetown", "Grass Patch", "Grass Valley",
	"Green Head", "Green Range", "Green Valley", "Greenbushes", "Greenfields", "Greenhills", "Greenmount", 
	"Greenough", "Greenwood", "Greenwoods Valley", "Gregory", "Greys Plain", "Grimwade", "Guilderton",
	"Guildford", "Gwambygine", "Gwelup", "Gwindinup", "Hacketts Gully", "Halls Creek", "Halls Head", "Hamel",
	"Hamersley", "Hamilton Hill", "Hammond Park", "Hannans", "Harris River", "Harrisdale", "Harrismith", "Harvey", 
	"Hastings", "Haynes", "Hazelmere", "Heathridge", "Helena Valley", "Henderson", "Henley Brook", "Herdsman", 
	"Herne Hill", "Herron", "Hester", "Hester Brook", "High Wycombe", "Highbury", "Highgate", "Hilbert",
	"Hillarys", "Hillman", "Hillside", "Hilton", "Hines Hill", "Hithergreen", "Hocking", "Hoddys Well", 
	"Hoffman", "Holleton", "Holt Rock", "Hope Valley", "Hopeland", "Hopetoun", "Horrocks", "Hovea", "Howatharra", 
	"Howick", "Huntingdale", "Hyden", "Iluka", "Inggarda", "Inglewood", "Innaloo", "Irishtown", "Irwin", 
	"Israelite Bay", "Jacup", "Jaloran", "Jandabup", "Jandakot", "Jane Brook", "Jardee", "Jarrahdale", 
	"Jarrahwood", "Jennacubbine", "Jennapullin", "Jerdacuttup", "Jerramungup", "Jibberding", "Jindalee", 
	"Jindong", "Jingalup", "Jolimont", "Joondalup", "Joondanna", "Kalamunda", "Kalannie", "Kalbarri", "Kalgan", 
	"Kalgoorlie", "Kalgup", "Kallaroo", "Kaloorup", "Kambalda East", "Kambalda West", "Kangaroo Gully", 
	"Karawara", "Kardinya", "Karlgarin", "Karlkurla", "Karloning", "Karloo", "Karnup", "Karragullen",
	"Karrakatta", "Karrakup", "Karratha", "Karridale", "Karrinyup", "Katanning", "Katrine", "Kealy", "Kebaringup",
	"Kellerberrin", "Kelmscott", "Kendenup", "Kensington", "Kenwick", "Kewdale", "Kiara", "King River",
	"Kingsford", "Kingsley", "Kingsway", "Kinross", "Kirup", "Kojarena", "Kojonup", "Kondinin", "Kondut",
	"Konnongorring", "Koojan", "Koolyanobbing", "Koondoola", "Koongamia", "Koorda", "Korrelocking", "Kronkup",
	"Kudardup", "Kuender", "Kukerin", "Kulikup", "Kulin", "Kulin West", "Kulja", "Kunjin", "Kununoppin", 
	"Kununurra", "Kweda", "Kwinana Beach", "Kwinana Town Centre", "Kwolyin", "Lagrange", "Lake Biddy", 
	"Lake Camm", "Lake Clifton", "Lake Grace", "Lake Hinds", "Lake King", "Lake Muir", "Lake Toolbrunup", 
	"Lakelands", "Lamington", "Lancelin", "Landsdale", "Lange", "Langford", "Latham", "Lathlain", "Learmonth", 
	"Leda", "Ledge Point", "Leederville", "Leeman", "Leeming", "Leinster", "Leonora", "Lesmurdie", "Lexia",
	"Linfarne", "Little Grove", "Lockridge", "Lockyer", "Lowden", "Lower Chittering", "Lower King", "Lowlands", 
	"Ludlow", "Lumeah", "Lynwood", "Macleod", "Maddington", "Madeley", "Madora Bay", "Madura", "Magitup", 
	"Mahogany Creek", "Mahomets Flats", "Maida Vale", "Malabaine", "Malaga", "Malebelling", "Malmalling", 
	"Mandogalup", "Mandurah", "Manjimup", "Manmanning", "Manning", "Manypeaks", "Marangaroo", "Marbelup",
	"Marble Bar", "Marchagee", "Mardella", "Margaret River", "Mariginiup", "Marmion", "Marne", "Marracoonda",
	"Marradong", "Marrinup", "Martin", "Marvel Loch", "Marybrook", "Massey Bay", "Mayanup", "Maylands", "Mckail",
	"Meadow Springs", "Meckering", "Medina", "Meekatharra", "Meelon", "Meenaar", "Melaleuca", "Melville",
	"Menora", "Merivale", "Merkanooka", "Merredin", "Merriwa", "Meru", "Metricup", "Mettler", "Middle Swan",
	"Middlesex", "Middleton Beach", "Midland", "Midvale", "Miling", "Millars Well", "Millbridge", "Millbrook",
	"Millendon", "Millstream", "Milpara", "Mindarabin", "Mindarie", "Mingenew", "Minigin", "Minnivale", "Minyirr", 
	"Mira Mar", "Mirrabooka", "Mobrup", "Mogumber", "Mokine", "Mollerin", "Molloy Island", "Monjebup", 
	"Monjingup", "Moodiarrup", "Moojebing", "Mooliabeenee", "Moonyoonooka", "Moora", "Moorine Rock", "Morangup", 
	"Morawa", "Mordalup", "Moresby", "Morgantown", "Morley", "Mosman Park", "Moulyinning", "Mount Barker",
	"Mount Claremont", "Mount Clarence", "Mount Elphinstone", "Mount Hardey", "Mount Hawthorn", "Mount Helena", 
	"Mount Lawley", "Mount Melville", "Mount Nasura", "Mount Pleasant", "Mount Richon", "Mount Tarcoola", 
	"Muchea", "Mukinbudin", "Mullaloo", "Mullalyup", "Mullewa", "Mullingar", "Muluckine", "Mumberkine",
	"Mundaring", "Mundijong", "Mundrabilla", "Mungalup", "Munglinup", "Munster", "Muntadgin", "Muradup",
	"Murdoch", "Murdong", "Muresk", "Myalup", "Myaree", "Myrup", "Nabawa", "Nairibin", "Nalkain", "Nambeelup", 
	"Nanarup", "Nangetty", "Nannup", "Napier", "Naraling", "Narembeen", "Narrikup", "Narrogin", "Narrogin Valley", 
	"Naval Base", "Nedlands", "Needilup", "Neerabup", "Neridup", "New Norcia", "Newdegate", "Newlands", "Newman",
	"Nippering", "Nirimba", "Noggerup", "Nokaning", "Nollamara", "Nomans Lake", "Noranda", "Nornalup",
	"Norseman", "North Beach", "North Coogee", "North Dandalup", "North Fremantle", "North Greenbushes", 
	"North Jindong", "North Lake", "North Perth", "North Plantations", "North West Cape", "North Yunderup", 
	"Northam", "Northampton", "Northbridge", "Northcliffe", "Northern Gully", "Nowergup", "Nukarni", "Nullagine",
	"Nullaki", "Nulsen", "Nunile", "Nyabing", "O'Connor", "Oakford", "Oakley", "Ocean Reef", "Ogilvie", "Oldbury",
	"Ongerup", "Onslow", "Ora Banda", "Orana", "Orange Grove", "Orchid Valley", "Ord River", "Orelia", 
	"Osborne Park", "Osmington", "Padbury", "Palgarup", "Palmyra", "Pannawonica", "Pantapin", "Paraburdoo",
	"Parkerville", "Parklands", "Parkwood", "Parmelia", "Parryville", "Paulls Valley", "Paynedale", "Paynes Find",
	"Peak Hill", "Pearsall", "Pegs Creek", "Pelican Point", "Pemberton", "Peppermint Grove", 
	"Peppermint Grove Beach", "Perenjori", "Perillup", "Peron", "Perth", "Perth Airport", "Perup",
	"Piara Waters", "Piawaning", "Piccadilly", "Pickering Brook", "Picton", "Piesse Brook", "Pindar", 
	"Pingaring", "Pingrup", "Pinjar", "Pinjarra", "Pink Lake", "Pinwernying", "Pithara", "Point Samson", 
	"Popanyinning", "Porongurup", "Port Albany", "Port Denison", "Port Hedland", "Port Kennedy", "Postans", 
	"Preston Beach", "Prevelly", "Pumphreys Bridge", "Quairading", "Qualeup", "Queens Park", "Quellington", 
	"Quindalup", "Quindanning", "Quinninup", "Quinns Rocks", "Rangeway", "Ravensthorpe", "Ravenswood", "Rawlinna",
	"Red Gully", "Red Hill", "Redbank", "Redcliffe", "Redmond", "Redmond West", "Regans Ford", "Reinscourt",
	"Ridgewood", "Ringbark", "Riverton", "Rivervale", "Robinson", "Rockingham", "Rockingham Beach", "Rocky Gully", 
	"Roebourne", "Roebuck", "Roelands", "Roleystone", "Rosa Brook", "Rosa Glen", "Rossmore", "Rossmoyne",
	"Ruabon", "Sabina River", "Safety Bay", "Salmon Gums", "Salter Point", "Samson", "San Remo", "Sawyers Valley",
	"Scaddan", "Scarborough", "Seabird", "Secret Harbour", "Seppings", "Serpentine", "Seville Grove", 
	"Shackleton", "Shannon", "Shark Bay", "Shelley", "Shenton Park", "Shoalwater", "Shotts", "Siesta Park", 
	"Silver Sands", "Sinagra", "Sinclair", "Singleton", "Smith Brook", "Somerville", "Sorrento", "South Bunbury",
	"South Carnarvon", "South Datatine", "South Fremantle", "South Glencoe", "South Guildford", "South Hedland",
	"South Kalgoorlie", "South Kumminin", "South Lake", "South Perth", "South Plantations", "South Stirling",
	"South Yunderup", "Southern Brook", "Southern Cross", "Southern River", "Spalding", "Spearwood", 
	"Spencer Park", "Spencers Brook", "St James", "Stake Hill", "Stirling", "Stirling Estate", "Stoneville",
	"Stove Hill", "Strathalbyn", "Stratton", "Subiaco", "Subiaco East", "Success", "Sunset Beach", "Swan View",
	"Swanbourne", "Takalarup", "Tamala Park", "Tambellup", "Tapping", "Tarcoola Beach", "Tarin Rock", "Telfer",
	"Tenindewa", "Tenterden", "The Lakes", "The Spectacles", "The Vines", "Thornlie", "Three Springs", 
	"Throssell", "Tincurrin", "Tom Price", "Tonebridge", "Toodyay", "Toolibin", "Torbay", "Torndirrup", 
	"Townsendale", "Trayning", "Trigg", "Trigwell", "Tuart Hill", "Tutunup", "Two Rocks", "Uduc", "Upper Swan",
	"Upper Warren", "Useless Loop", "Usher", "Utakarra", "Varley", "Vasse", "Victoria Park", "Vittoria", 
	"Viveash", "Waddington", "Wagerup", "Waggrakine", "Wagin", "Waikiki", "Walebing", "Walgoolan", "Walkaway", 
	"Walliston", "Walmsley", "Walpole", "Walsall", "Walyurin", "Wamenusking", "Wandering", "Wandi", "Wandina", 
	"Wangara", "Wannamal", "Wannanup", "Wanneroo", "Warawarrup", "Warnbro", "Waroona", "Warradarge", "Warralakin",
	"Warrenup", "Warwick", "Waterbank", "Waterford", "Waterloo", "Watermans Bay", "Watheroo", "Wattening", 
	"Wattle Grove", "Wattleup", "Webberton", "Wedgefield", "Welbungin", "Wellard", "Wellington Mill", "Wellstead",
	"Welshpool", "Wembley", "Wembley Downs", "West Beach", "West Busselton", "West End", "West Kalgoorlie",
	"West Lamington", "West Leederville", "West Perth", "West Pinjarra", "West River", "West Swan", "West Toodyay",
	"Westdale", "Westminster", "Westonia", "Whim Creek", "Whitby", "White Gum Valley", "Whiteman", "Whittaker", 
	"Wialki", "Wickepin", "Wickham", "Widgiemooltha", "Wilga", "Wilgarrup", "Wilgoyne", "Willagee", "Willetton",
	"Williams", "Williamstown", "Willyung", "Wilson", "Wilyabrup", "Windabout", "Winnejup", "Winthrop",
	"Witchcliffe", "Withers", "Wittenoom", "Wokalup", "Wongamine", "Wongan Hills", "Wonnerup", "Wonthella",
	"Woodanilling", "Woodbridge", "Woodlands", "Woodvale", "Wooroloo", "Woorree", "Worsley", "Wubin", "Wundowie",
	"Wungong", "Wyalkatchem", "Wyening", "Wyndham", "Xantippe", "Yakamia", "Yalgoo", "Yallingup", "Yalyalup",
	"Yanchep", "Yandanooka", "Yangebup", "Yanmah", "Yarloop", "Yealering", "Yelbeni", "Yellowdine", "Yelverton",
	"Yerecoin", "Yilkari", "Yilliminning", "Yokine", "Yokine South", "Yoongarillup", "York", "Yornup", "Yoting",
	"Youndegin", "Youngs Siding", "Yuna"
};

const char *names_venue[] = 
{
	"Oval", "Dome", "Stadium", "Arena", "Thunderdome", "Field", "Pitch", "Grounds", "Gulley",
	"Square", "Yard", "College", "Community Centre", "Shopping City", "Library", "Abattoir"
};

void persondata_generate(persondata_t *output)
{
	memset(output,0, sizeof(*output));
	
	int fore = statfunc_rand_32b() % (sizeof(names_forename)/sizeof(names_forename[0]));
	int sur = statfunc_rand_32b() % (sizeof(names_surname)/sizeof(names_surname[0]));
	snprintf(output->name[0], sizeof(output->name[0])-1, "%s", names_forename[fore]);
	snprintf(output->name[1], sizeof(output->name[1])-1, "%s", names_surname[sur]);
	
	output->height = statfunc_gauss_8b(170, 8);
	output->mass = statfunc_gauss_8b(65, 10);

	output->runspeed = statfunc_gauss_8b(106, 40);
	output->sprintspeed = statfunc_gauss_8b(160, 43);
	output->ballspeed = statfunc_gauss_8b(100, 40);	
	if(output->ballspeed + 5 > output->runspeed)
		output->ballspeed = output->runspeed - 5;
	if(output->sprintspeed - 5 < output->runspeed)
		output->sprintspeed = output->runspeed + 5;

	output->staminamax = statfunc_gauss_8b(100, 10);
	output->staminarecover = statfunc_gauss_8b(100, 10);
	
	output->kickpower = statfunc_gauss_8b(100, 10);
	output->accuracy = statfunc_gauss_8b(100, 10);
	output->throwpower = statfunc_gauss_8b(100, 10);
	
	output->dive = statfunc_gauss_8b(100, 10);
	output->eyesight = statfunc_gauss_8b(100, 10);
	output->reaction = statfunc_gauss_8b(100, 10);
	
	output->hpmax = statfunc_gauss_8b(100, 10);
	output->hprecover = statfunc_gauss_8b(100, 10);
	
	output->salary = 0;
	output->salary += output->height * output->mass;
	output->salary += output->kickpower * output->accuracy;
	output->salary += output->hpmax * output->staminamax;
	output->salary += statfunc_gauss_8b(100, 100);
	
	output->look = statfunc_rand_8b();
	
}

void teamdata_generate(teamdata_t *output)
{
	memset(output, 0, sizeof(*output));
	
	int nn = statfunc_rand_32b() % (sizeof(names_mammal)/sizeof(names_mammal[0]));
	snprintf(output->name, sizeof(output->name)-1, "%s", names_mammal[nn]);
	
	int cc = statfunc_rand_32b() % (sizeof(names_city)/sizeof(names_city[0]));
	snprintf(output->city, sizeof(output->city)-1, "%s", names_city[cc]);
	
	int vv = statfunc_rand_32b() % (sizeof(names_venue)/sizeof(names_venue[0]));
	snprintf(output->venue, sizeof(output->venue)-1, "%s %s", names_city[cc], names_venue[vv]);
	
	for(unsigned pp = 0; pp < sizeof(output->persons)/sizeof(output->persons[0]); pp++)
	{
		persondata_generate(&(output->persons[pp]));
	}
	
	output->money = statfunc_gauss_8b(100, 20) * 100000;
	output->hype = statfunc_gauss_8b(100, 10);
	output->scandal = statfunc_gauss_8b(100, 10);
}


void leaguedata_generate(leaguedata_t *output)
{
	memset(output, 0, sizeof(*output));
	
	for(unsigned tt = 0; tt < sizeof(output->teams)/sizeof(output->teams[0]); tt++)
	{
		teamdata_generate(&(output->teams[tt]));
	}
}

leaguedata_t leaguedata_quick;
