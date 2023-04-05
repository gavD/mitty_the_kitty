Mitty The Kitty: Arduboy 2d game
==

(c) 2023 Gavin Davies, "Bean" Davies and "Button" Davies

My kids (5 and 4 at the time) designed this game and did most of the sprite work. I've withheld their names for their privacy but it was fun to work on this project with them!

This is a simple platform game for the Arduboy platform. Can you guide Mitty the Kitty past obstacles and enemies to beat all 8 levels? A "duff" is "Bean"'s parlance for hitting something! Mitty is one of my kids' cat teddies.

Plaforms
--

Developed on Mac and Windows using an Arduboy FX.

Target platform: [Arduboy](https://www.arduboy.com/)

Mapping of positions to pixels
--

Some objects in the game have x,y positions that are uint16_t rather than integer positions that correspond to
pixels. This is because the game runs at 60fps and you can't render sprites at a subpixel location (AFAIK! This
is my first proper Arduboy game! There's likely a better way). So, we store x,y at higher resolution then use a
multiplier to map between positions and pixel locations. This allows objects to move at different rates (e.g.
Mitty is faster than the squirrel, and the bird is faster than Mitty)

Why did you commit binaries, do you no git gud?
--

Well, I MOSTLY git gud, [read my book on Git if you like](https://gavd.co.uk/2021/09/book-git-workflow-discipline/).

The binaries are committed to make it easier to play it on an emulator - it's simple hosting without faffing around. This is a hobby; I'm on a tight timescale as a parent of small kids!

Although if there's a nigh-zero effort better way to publish a binary that I can easily update, lemme know!

How do I play this on a real Arduboy?
--

See:

- https://community.arduboy.com/t/how-to-add-games-to-an-arduboy/1423
- https://community.arduboy.com/t/how-do-i-install-a-game-on-arduboy/4798
- https://www.instructables.com/How-to-Upload-Games-to-Arduboy-and-500-Games-to-Fl/

License
--

Public domain
