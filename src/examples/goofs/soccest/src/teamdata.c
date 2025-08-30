//teamdata.c
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
	"ABBA RIVER", "ABBEY", "ACTON PARK", "AJANA", "ALBANY", "ALDERSYDE", "ALEXANDER HEIGHTS", "ALEXANDRA BRIDGE",
	"ALFRED COVE", "ALKIMOS", "ALLANOOKA", "ALLANSON", "AMBERGATE", "AMELUP", "ANKETELL", "ANNIEBROOK",
	"APPLECROSS", "APPLECROSS NORTH", "ARDATH", "ARDROSS", "ARGYLE", "ARMADALE", "ARRINO", "ARROWSMITH", 
	"ARTHUR RIVER", "ASCOT", "ASHBY", "ASHFIELD", "ATTADALE", "ATWELL", "AUBIN GROVE", "AUGUSTA", "AUSTRALIND",
	"AVELEY", "BAANDEE", "BABAKIN", "BABBAGE ISLAND", "BADGEBUP", "BADGERIN ROCK", "BADGIN", "BADGINGARRA",
	"BAKERS HILL", "BALBARRUP", "BALCATTA", "BALDIVIS", "BALGA", "BALINGUP", "BALKULING", "BALLADONIA", 
	"BALLAJURA", "BALLAYING", "BALLIDU", "BALLY BALLY", "BANDY CREEK", "BANJUP", "BANKSIA GROVE", 
	"BARRAGUP", "BASKERVILLE", "BASSENDEAN", "BATEMAN", "BAYONET HEAD", "BAYSWATER", "BEACHLANDS", 
	"BEACON", "BEACONSFIELD", "BEAUFORT RIVER", "BEAUMONT", "BECKENHAM", "BEDFORD", "BEDFORDALE", 
	"BEECHBORO", "BEECHINA", "BEEDELUP", "BEELA", "BEELERUP", "BEELIAR", "BEENONG", "BEERMULLAH",
	"BEJOORDING", "BELDON", "BELHUS", "BELLEVUE", "BELMONT", "BENCUBBIN", "BENGER", "BENJABERRING", "BENJINUP",
	"BENNETT SPRINGS", "BENTLEY", "BERESFORD", "BERTRAM", "BEVERLEY", "BIBRA LAKE", "BICKLEY", "BICTON", 
	"BIG GROVE", "BILBARIN", "BILINGURR", "BINDI BINDI", "BINDOON", "BINNINGUP", "BLUFF POINT", "BLYTHEWOOD", 
	"BOALLIA", "BODALLIN", "BODDINGTON", "BOKAL", "BOLGART", "BONNIE ROCK", "BOODARIE", "BOORAGOON", "BORDEN", 
	"BORNHOLM", "BOSCABEL", "BOULDER", "BOUNDAIN", "BOUVARD", "BOVELL", "BOW BRIDGE", "BOWELLING", "BOYA", 
	"BOYANUP", "BOYATUP", "BOYERINE", "BOYUP BROOK", "BRAMLEY", "BREMER BAY", "BRENTWOOD", "BRIDGETOWN", 
	"BRIGADOON", "BROADWATER", "BROADWAY NEDLANDS", "BROADWOOD", "BROCKMAN", "BROOKDALE", "BROOKHAMPTON",
	"BROOKTON", "BROOME", "BROOMEHILL", "BROOMEHILL EAST", "BROWN RANGE", "BRUCE ROCK", "BRUNSWICK", "BUCKINGHAM",
	"BUCKLAND", "BULGARRA", "BULL CREEK", "BULLARING", "BULLFINCH", "BULLOCK HILLS", "BULLSBROOK", "BUNBURY", 
	"BUNTINE", "BURAKIN", "BUREKUP", "BURLONG", "BURNS BEACH", "BURNSIDE", "BURRACOPPIN", "BURSWOOD", "BUSSELTON",
	"BUTLER", "BYFORD", "CADOUX", "CAIGUNA", "CALINGIRI", "CALISTA", "CAMBALLIN", "CAMILLO", 
	"CANNING BRIDGE APPLECROSS", "CANNING MILLS", "CANNING VALE", "CANNING VALE EAST", "CANNING VALE SOUTH", 
	"CANNINGTON", "CAPE RANGE NATIONAL PARK", "CAPEL", "CAPEL RIVER", "CARABOODA", "CARANI", "CARBUNUP RIVER", 
	"CARDIFF", "CARDUP", "CAREY PARK", "CARINE", "CARLISLE", "CARLISLE NORTH", "CARLISLE SOUTH", "CARMEL", 
	"CARNAMAH", "CARNARVON", "CARRAMAR", "CARROLUP", "CARTMETICUP", "CASCADE", "CASTLETOWN", "CASUARINA",
	"CATABY", "CATTERICK", "CAVERSHAM", "CENTENNIAL PARK", "CERVANTES", "CHADWICK", "CHAMPION LAKES", 
	"CHAPMAN HILL", "CHIDLOW", "CHITTERING", "CHURCHLANDS", "CITY BEACH", "CLACKLINE", "CLAREMONT", 
	"CLAREMONT NORTH", "CLARKSON", "CLOVERDALE", "COBLININE", "COCKBURN CENTRAL", "COLLANILLING", "COLLEGE GROVE",
	"COLLIE", "COLLIE BURN", "COLLINGWOOD HEIGHTS", "COLLINGWOOD PARK", "COMO", "CONDINGUP", "CONNOLLY", 
	"CONTINE", "COODANUP", "COOGEE", "COOKERNUP", "COOLBELLUP", "COOLBINIA", "COOLGARDIE", "COOLOONGUP", 
	"COOLUP", "COOMALBIDGUP", "COOMBERDALE", "COONDLE", "COOROW", "CORAL BAY", "CORRIGIN", "COTTESLOE", 
	"COWARAMUP", "COWCOWING", "COYRECUP", "CRAIGIE", "CRANBROOK", "CRAWLEY", "CROWEA", "CUBALLING", "CUE",
	"CULHAM", "CULLACABARDEE", "CUNDERDIN", "CUNDINUP", "CUNJARDINE", "CURRAMBINE", "CUTHBERT", "DAGLISH",
	"DALKEITH", "DALWALLINU", "DALYELLUP", "DALYUP", "DAMPIER", "DAMPIER PENINSULA", "DANDARAGAN", "DANGIN", 
	"DARCH", "DARDADINE", "DARDANUP", "DARDANUP WEST", "DARKAN", "DARLING DOWNS", "DARLINGTON", "DATATINE", 
	"DAVENPORT", "DAWESVILLE", "DEANMILL", "DENBARKER", "DENHAM", "DENMARK", "DERBY", "DIAMOND TREE", "DIANELLA",
	"DINGUP", "DINNINUP", "DIXVALE", "DJUGUN", "DONGARA", "DONGOLOCKING", "DONNELLY RIVER", "DONNYBROOK", 
	"DOODLAKINE", "DOUBLEVIEW", "DOWERIN", "DROME", "DRUMMOND COVE", "DRYANDRA", "DUDININ", "DUDLEY PARK",
	"DUKIN", "DULYALBIN", "DUMBARTON", "DUMBERNING", "DUMBLEYUNG", "DUNCRAIG", "DUNDAS", "DUNSBOROUGH", 
	"DURANILLIN", "DWELLINGUP", "EAGLE BAY", "EAST BUNBURY", "EAST CANNINGTON", "EAST CARNARVON",
	"EAST FREMANTLE", "EAST MUNGLINUP", "EAST PERTH", "EAST ROCKINGHAM", "EAST VICTORIA PARK", "EATON",
	"EDEN HILL", "EDGEWATER", "EGLINTON", "EIGHTY MILE BEACH", "ELGIN", "ELLEKER", "ELLENBROOK", "EMBLETON",
	"EMU POINT", "ENEABBA", "ERADU", "ERSKINE", "ESPERANCE", "EUCLA", "EWLYAMARTUP", "EXMOUTH", "EXMOUTH GULF", 
	"FAIRBRIDGE", "FALCON", "FERGUSON", "FERNDALE", "FITZGERALD", "FITZROY CROSSING", "FLOREAT", "FOREST GROVE",
	"FORREST", "FORREST BEACH", "FORRESTDALE", "FORRESTFIELD", "FRASER RANGE", "FREMANTLE", "FRENCHMAN BAY", 
	"FURNISSDALE", "GABBIN", "GAIRDNER", "GELORUP", "GEOGRAPHE", "GERALDTON", "GHOOLI", "GIBSON", "GILLINGARRA", 
	"GINGIN", "GIRRAWHEEN", "GLEDHOW", "GLEN FORREST", "GLEN IRIS", "GLENDALOUGH", "GLENFIELD", "GLENORAN", 
	"GNANGARA", "GNARABUP", "GNOWANGERUP", "GNOWELLEN", "GOLDEN BAY", "GOODE BEACH", "GOODLANDS", "GOOMALLING",
	"GOOMARIN", "GOOSEBERRY HILL", "GORGE ROCK", "GORRIE", "GOSNELLS", "GRACETOWN", "GRASS PATCH", "GRASS VALLEY",
	"GREEN HEAD", "GREEN RANGE", "GREEN VALLEY", "GREENBUSHES", "GREENFIELDS", "GREENHILLS", "GREENMOUNT", 
	"GREENOUGH", "GREENWOOD", "GREENWOODS VALLEY", "GREGORY", "GREYS PLAIN", "GRIMWADE", "GUILDERTON",
	"GUILDFORD", "GWAMBYGINE", "GWELUP", "GWINDINUP", "HACKETTS GULLY", "HALLS CREEK", "HALLS HEAD", "HAMEL",
	"HAMERSLEY", "HAMILTON HILL", "HAMMOND PARK", "HANNANS", "HARRIS RIVER", "HARRISDALE", "HARRISMITH", "HARVEY", 
	"HASTINGS", "HAYNES", "HAZELMERE", "HEATHRIDGE", "HELENA VALLEY", "HENDERSON", "HENLEY BROOK", "HERDSMAN", 
	"HERNE HILL", "HERRON", "HESTER", "HESTER BROOK", "HIGH WYCOMBE", "HIGHBURY", "HIGHGATE", "HILBERT",
	"HILLARYS", "HILLMAN", "HILLSIDE", "HILTON", "HINES HILL", "HITHERGREEN", "HOCKING", "HODDYS WELL", 
	"HOFFMAN", "HOLLETON", "HOLT ROCK", "HOPE VALLEY", "HOPELAND", "HOPETOUN", "HORROCKS", "HOVEA", "HOWATHARRA", 
	"HOWICK", "HUNTINGDALE", "HYDEN", "ILUKA", "INGGARDA", "INGLEWOOD", "INNALOO", "IRISHTOWN", "IRWIN", 
	"ISRAELITE BAY", "JACUP", "JALORAN", "JANDABUP", "JANDAKOT", "JANE BROOK", "JARDEE", "JARRAHDALE", 
	"JARRAHWOOD", "JENNACUBBINE", "JENNAPULLIN", "JERDACUTTUP", "JERRAMUNGUP", "JIBBERDING", "JINDALEE", 
	"JINDONG", "JINGALUP", "JOLIMONT", "JOONDALUP", "JOONDANNA", "KALAMUNDA", "KALANNIE", "KALBARRI", "KALGAN", 
	"KALGOORLIE", "KALGUP", "KALLAROO", "KALOORUP", "KAMBALDA EAST", "KAMBALDA WEST", "KANGAROO GULLY", 
	"KARAWARA", "KARDINYA", "KARLGARIN", "KARLKURLA", "KARLONING", "KARLOO", "KARNUP", "KARRAGULLEN",
	"KARRAKATTA", "KARRAKUP", "KARRATHA", "KARRIDALE", "KARRINYUP", "KATANNING", "KATRINE", "KEALY", "KEBARINGUP",
	"KELLERBERRIN", "KELMSCOTT", "KENDENUP", "KENSINGTON", "KENWICK", "KEWDALE", "KIARA", "KING RIVER",
	"KINGSFORD", "KINGSLEY", "KINGSWAY", "KINROSS", "KIRUP", "KOJARENA", "KOJONUP", "KONDININ", "KONDUT",
	"KONNONGORRING", "KOOJAN", "KOOLYANOBBING", "KOONDOOLA", "KOONGAMIA", "KOORDA", "KORRELOCKING", "KRONKUP",
	"KUDARDUP", "KUENDER", "KUKERIN", "KULIKUP", "KULIN", "KULIN WEST", "KULJA", "KUNJIN", "KUNUNOPPIN", 
	"KUNUNURRA", "KWEDA", "KWINANA BEACH", "KWINANA TOWN CENTRE", "KWOLYIN", "LAGRANGE", "LAKE BIDDY", 
	"LAKE CAMM", "LAKE CLIFTON", "LAKE GRACE", "LAKE HINDS", "LAKE KING", "LAKE MUIR", "LAKE TOOLBRUNUP", 
	"LAKELANDS", "LAMINGTON", "LANCELIN", "LANDSDALE", "LANGE", "LANGFORD", "LATHAM", "LATHLAIN", "LEARMONTH", 
	"LEDA", "LEDGE POINT", "LEEDERVILLE", "LEEMAN", "LEEMING", "LEINSTER", "LEONORA", "LESMURDIE", "LEXIA",
	"LINFARNE", "LITTLE GROVE", "LOCKRIDGE", "LOCKYER", "LOWDEN", "LOWER CHITTERING", "LOWER KING", "LOWLANDS", 
	"LUDLOW", "LUMEAH", "LYNWOOD", "MACLEOD", "MADDINGTON", "MADELEY", "MADORA BAY", "MADURA", "MAGITUP", 
	"MAHOGANY CREEK", "MAHOMETS FLATS", "MAIDA VALE", "MALABAINE", "MALAGA", "MALEBELLING", "MALMALLING", 
	"MANDOGALUP", "MANDURAH", "MANJIMUP", "MANMANNING", "MANNING", "MANYPEAKS", "MARANGAROO", "MARBELUP",
	"MARBLE BAR", "MARCHAGEE", "MARDELLA", "MARGARET RIVER", "MARIGINIUP", "MARMION", "MARNE", "MARRACOONDA",
	"MARRADONG", "MARRINUP", "MARTIN", "MARVEL LOCH", "MARYBROOK", "MASSEY BAY", "MAYANUP", "MAYLANDS", "MCKAIL",
	"MEADOW SPRINGS", "MECKERING", "MEDINA", "MEEKATHARRA", "MEELON", "MEENAAR", "MELALEUCA", "MELVILLE",
	"MENORA", "MERIVALE", "MERKANOOKA", "MERREDIN", "MERRIWA", "MERU", "METRICUP", "METTLER", "MIDDLE SWAN",
	"MIDDLESEX", "MIDDLETON BEACH", "MIDLAND", "MIDVALE", "MILING", "MILLARS WELL", "MILLBRIDGE", "MILLBROOK",
	"MILLENDON", "MILLSTREAM", "MILPARA", "MINDARABIN", "MINDARIE", "MINGENEW", "MINIGIN", "MINNIVALE", "MINYIRR", 
	"MIRA MAR", "MIRRABOOKA", "MOBRUP", "MOGUMBER", "MOKINE", "MOLLERIN", "MOLLOY ISLAND", "MONJEBUP", 
	"MONJINGUP", "MOODIARRUP", "MOOJEBING", "MOOLIABEENEE", "MOONYOONOOKA", "MOORA", "MOORINE ROCK", "MORANGUP", 
	"MORAWA", "MORDALUP", "MORESBY", "MORGANTOWN", "MORLEY", "MOSMAN PARK", "MOULYINNING", "MOUNT BARKER",
	"MOUNT CLAREMONT", "MOUNT CLARENCE", "MOUNT ELPHINSTONE", "MOUNT HARDEY", "MOUNT HAWTHORN", "MOUNT HELENA", 
	"MOUNT LAWLEY", "MOUNT MELVILLE", "MOUNT NASURA", "MOUNT PLEASANT", "MOUNT RICHON", "MOUNT TARCOOLA", 
	"MUCHEA", "MUKINBUDIN", "MULLALOO", "MULLALYUP", "MULLEWA", "MULLINGAR", "MULUCKINE", "MUMBERKINE",
	"MUNDARING", "MUNDIJONG", "MUNDRABILLA", "MUNGALUP", "MUNGLINUP", "MUNSTER", "MUNTADGIN", "MURADUP",
	"MURDOCH", "MURDONG", "MURESK", "MYALUP", "MYAREE", "MYRUP", "NABAWA", "NAIRIBIN", "NALKAIN", "NAMBEELUP", 
	"NANARUP", "NANGETTY", "NANNUP", "NAPIER", "NARALING", "NAREMBEEN", "NARRIKUP", "NARROGIN", "NARROGIN VALLEY", 
	"NAVAL BASE", "NEDLANDS", "NEEDILUP", "NEERABUP", "NERIDUP", "NEW NORCIA", "NEWDEGATE", "NEWLANDS", "NEWMAN",
	"NIPPERING", "NIRIMBA", "NOGGERUP", "NOKANING", "NOLLAMARA", "NOMANS LAKE", "NORANDA", "NORNALUP",
	"NORSEMAN", "NORTH BEACH", "NORTH COOGEE", "NORTH DANDALUP", "NORTH FREMANTLE", "NORTH GREENBUSHES", 
	"NORTH JINDONG", "NORTH LAKE", "NORTH PERTH", "NORTH PLANTATIONS", "NORTH WEST CAPE", "NORTH YUNDERUP", 
	"NORTHAM", "NORTHAMPTON", "NORTHBRIDGE", "NORTHCLIFFE", "NORTHERN GULLY", "NOWERGUP", "NUKARNI", "NULLAGINE",
	"NULLAKI", "NULSEN", "NUNILE", "NYABING", "O'CONNOR", "OAKFORD", "OAKLEY", "OCEAN REEF", "OGILVIE", "OLDBURY",
	"ONGERUP", "ONSLOW", "ORA BANDA", "ORANA", "ORANGE GROVE", "ORCHID VALLEY", "ORD RIVER", "ORELIA", 
	"OSBORNE PARK", "OSMINGTON", "PADBURY", "PALGARUP", "PALMYRA", "PANNAWONICA", "PANTAPIN", "PARABURDOO",
	"PARKERVILLE", "PARKLANDS", "PARKWOOD", "PARMELIA", "PARRYVILLE", "PAULLS VALLEY", "PAYNEDALE", "PAYNES FIND",
	"PEAK HILL", "PEARSALL", "PEGS CREEK", "PELICAN POINT", "PEMBERTON", "PEPPERMINT GROVE", 
	"PEPPERMINT GROVE BEACH", "PERENJORI", "PERILLUP", "PERON", "PERTH", "PERTH AIRPORT", "PERUP",
	"PIARA WATERS", "PIAWANING", "PICCADILLY", "PICKERING BROOK", "PICTON", "PIESSE BROOK", "PINDAR", 
	"PINGARING", "PINGRUP", "PINJAR", "PINJARRA", "PINK LAKE", "PINWERNYING", "PITHARA", "POINT SAMSON", 
	"POPANYINNING", "PORONGURUP", "PORT ALBANY", "PORT DENISON", "PORT HEDLAND", "PORT KENNEDY", "POSTANS", 
	"PRESTON BEACH", "PREVELLY", "PUMPHREYS BRIDGE", "QUAIRADING", "QUALEUP", "QUEENS PARK", "QUELLINGTON", 
	"QUINDALUP", "QUINDANNING", "QUINNINUP", "QUINNS ROCKS", "RANGEWAY", "RAVENSTHORPE", "RAVENSWOOD", "RAWLINNA",
	"RED GULLY", "RED HILL", "REDBANK", "REDCLIFFE", "REDMOND", "REDMOND WEST", "REGANS FORD", "REINSCOURT",
	"RIDGEWOOD", "RINGBARK", "RIVERTON", "RIVERVALE", "ROBINSON", "ROCKINGHAM", "ROCKINGHAM BEACH", "ROCKY GULLY", 
	"ROEBOURNE", "ROEBUCK", "ROELANDS", "ROLEYSTONE", "ROSA BROOK", "ROSA GLEN", "ROSSMORE", "ROSSMOYNE",
	"RUABON", "SABINA RIVER", "SAFETY BAY", "SALMON GUMS", "SALTER POINT", "SAMSON", "SAN REMO", "SAWYERS VALLEY",
	"SCADDAN", "SCARBOROUGH", "SEABIRD", "SECRET HARBOUR", "SEPPINGS", "SERPENTINE", "SEVILLE GROVE", 
	"SHACKLETON", "SHANNON", "SHARK BAY", "SHELLEY", "SHENTON PARK", "SHOALWATER", "SHOTTS", "SIESTA PARK", 
	"SILVER SANDS", "SINAGRA", "SINCLAIR", "SINGLETON", "SMITH BROOK", "SOMERVILLE", "SORRENTO", "SOUTH BUNBURY",
	"SOUTH CARNARVON", "SOUTH DATATINE", "SOUTH FREMANTLE", "SOUTH GLENCOE", "SOUTH GUILDFORD", "SOUTH HEDLAND",
	"SOUTH KALGOORLIE", "SOUTH KUMMININ", "SOUTH LAKE", "SOUTH PERTH", "SOUTH PLANTATIONS", "SOUTH STIRLING",
	"SOUTH YUNDERUP", "SOUTHERN BROOK", "SOUTHERN CROSS", "SOUTHERN RIVER", "SPALDING", "SPEARWOOD", 
	"SPENCER PARK", "SPENCERS BROOK", "ST JAMES", "STAKE HILL", "STIRLING", "STIRLING ESTATE", "STONEVILLE",
	"STOVE HILL", "STRATHALBYN", "STRATTON", "SUBIACO", "SUBIACO EAST", "SUCCESS", "SUNSET BEACH", "SWAN VIEW",
	"SWANBOURNE", "TAKALARUP", "TAMALA PARK", "TAMBELLUP", "TAPPING", "TARCOOLA BEACH", "TARIN ROCK", "TELFER",
	"TENINDEWA", "TENTERDEN", "THE LAKES", "THE SPECTACLES", "THE VINES", "THORNLIE", "THREE SPRINGS", 
	"THROSSELL", "TINCURRIN", "TOM PRICE", "TONEBRIDGE", "TOODYAY", "TOOLIBIN", "TORBAY", "TORNDIRRUP", 
	"TOWNSENDALE", "TRAYNING", "TRIGG", "TRIGWELL", "TUART HILL", "TUTUNUP", "TWO ROCKS", "UDUC", "UPPER SWAN",
	"UPPER WARREN", "USELESS LOOP", "USHER", "UTAKARRA", "VARLEY", "VASSE", "VICTORIA PARK", "VITTORIA", 
	"VIVEASH", "WADDINGTON", "WAGERUP", "WAGGRAKINE", "WAGIN", "WAIKIKI", "WALEBING", "WALGOOLAN", "WALKAWAY", 
	"WALLISTON", "WALMSLEY", "WALPOLE", "WALSALL", "WALYURIN", "WAMENUSKING", "WANDERING", "WANDI", "WANDINA", 
	"WANGARA", "WANNAMAL", "WANNANUP", "WANNEROO", "WARAWARRUP", "WARNBRO", "WAROONA", "WARRADARGE", "WARRALAKIN",
	"WARRENUP", "WARWICK", "WATERBANK", "WATERFORD", "WATERLOO", "WATERMANS BAY", "WATHEROO", "WATTENING", 
	"WATTLE GROVE", "WATTLEUP", "WEBBERTON", "WEDGEFIELD", "WELBUNGIN", "WELLARD", "WELLINGTON MILL", "WELLSTEAD",
	"WELSHPOOL", "WEMBLEY", "WEMBLEY DOWNS", "WEST BEACH", "WEST BUSSELTON", "WEST END", "WEST KALGOORLIE",
	"WEST LAMINGTON", "WEST LEEDERVILLE", "WEST PERTH", "WEST PINJARRA", "WEST RIVER", "WEST SWAN", "WEST TOODYAY",
	"WESTDALE", "WESTMINSTER", "WESTONIA", "WHIM CREEK", "WHITBY", "WHITE GUM VALLEY", "WHITEMAN", "WHITTAKER", 
	"WIALKI", "WICKEPIN", "WICKHAM", "WIDGIEMOOLTHA", "WILGA", "WILGARRUP", "WILGOYNE", "WILLAGEE", "WILLETTON",
	"WILLIAMS", "WILLIAMSTOWN", "WILLYUNG", "WILSON", "WILYABRUP", "WINDABOUT", "WINNEJUP", "WINTHROP",
	"WITCHCLIFFE", "WITHERS", "WITTENOOM", "WOKALUP", "WONGAMINE", "WONGAN HILLS", "WONNERUP", "WONTHELLA",
	"WOODANILLING", "WOODBRIDGE", "WOODLANDS", "WOODVALE", "WOOROLOO", "WOORREE", "WORSLEY", "WUBIN", "WUNDOWIE",
	"WUNGONG", "WYALKATCHEM", "WYENING", "WYNDHAM", "XANTIPPE", "YAKAMIA", "YALGOO", "YALLINGUP", "YALYALUP",
	"YANCHEP", "YANDANOOKA", "YANGEBUP", "YANMAH", "YARLOOP", "YEALERING", "YELBENI", "YELLOWDINE", "YELVERTON",
	"YERECOIN", "YILKARI", "YILLIMINNING", "YOKINE", "YOKINE SOUTH", "YOONGARILLUP", "YORK", "YORNUP", "YOTING",
	"YOUNDEGIN", "YOUNGS SIDING", "YUNA"
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
	
	for(unsigned pp = 0; pp < sizeof(output->persons)/sizeof(output->persons[0]); pp++)
	{
		persondata_generate(&(output->persons[pp]));
	}
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
