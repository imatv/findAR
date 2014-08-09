/**
 * findAR
 * Oh, there you AR.
 */

var UI = require('ui');
var ajax = require('ajax');
var Accel = require('ui/accel');
var Vector2 = require('vector2');

function init() {
  Accel.init();
  
  Accel.on('tap', function(e) {
    sendCmd('Hue Scan');
    console.log('Tap event on axis: ' + e.axis + ' and direction: ' + e.direction);
  });
}
init();

function sendCmd(cmd) {
  var _cmd = cmd.toLowerCase().trim();
  
  ajax({
      url: 'http://dev.quasi.co/findar/in.php?cmd='+_cmd,
      cache: false
    },
    function(data) {
      console.log('ajax response: ' + data);
    },
    function(error) {
      console.log('The ajax request failed: ' + error);
    });
  }

function printObject(o) {
  var out = '';
  for (var p in o) {
    out += p + ': ' + o[p] + '\n';
  }
  return out;
}

var main = new UI.Card({
  title: 'findAR',
  subtitle: 'Oh, there you AR.',
  body: 'Press any button.'
});

main.show();

main.on('click', 'up', function(e) {
  
  var modesMenuItems = [{
              title: 'Original'
            },{
              title: 'Outline'
            },{
              title: 'Grayscale'
            },{
              title: 'B/W'
            },{
              title: 'Sepia'
            },{
              title: 'Hue Scan'
            }];  
  var modesMenu = new UI.Menu({
    name: 'mode',
          sections: [{
            items: modesMenuItems
          }]});
  
  var colorMenuItems = [{
              title: '+ Hue'
            },{
              title: '- Hue'
            },{
              title: '- Saturation'
            },{
              title: '+ Saturation'
            },{
              title: '- Lightness'
            },{
              title: '+ Lightness'
            }];
  var colorMenu = new UI.Menu({
    name: 'color',
          sections: [{ // HSL
            items: colorMenuItems
          }]
        });
  
  // top level menu
  var _items = [{
        title: 'Modes',
        subtitle: 'All the modes',
        card: null,
        menu: modesMenu
      }, {
        title: 'Color Picker',
        subtitle: 'Choose wisely',
        card: null,
        menu: colorMenu
      },
      {
        title: 'Extra',
        subtitle: 'Misc stuff',
        card: {
          body: 'Coming soon...'
        }
      },
      {
        title: 'Credits',
        subtitle: 'Code, etc.',
        card: {
          body: 'Ethan_Hart,Alvin_Vuong,Yuki_Pan,Mike_Blix'
        }
      }
  ];
  
  modesMenu.on('select', function(ev){
    console.log(printObject(ev));
    console.log('selected item #: ' + ev.item);
    
    var it = modesMenuItems[ev.item];
    
    console.log(it.title);

    //console.log('action select: ' + _items[ev.item].title);
    sendCmd(it.title);
  });
  
  colorMenu.on('select', function(ev){
    console.log(printObject(ev));
    console.log('selected item #: ' + ev.item);
    
    var it = colorMenuItems[ev.item];
    
    console.log(it.title);
    
    sendCmd(it.title);
  });
  
  var menu = new UI.Menu({
    sections: [{
      items: _items
    }]
  });
  menu.on('select', function(e) {
    console.log('Selected item: ' + e.section + ' ' + e.item);
    
    var item = _items[e.item];
    
    if (item.menu !== undefined) {
      console.log('show menu');
      
      var _menu = item.menu;
      
      /*_menu.on('select', function(ev){
        console.log(printObject(ev));
        
        var _items = _menu.items;
        
        console.log('action select: ' + _items[ev.item].title);
        // ajax();
      });*/
      
      _menu.show();
      
    } else { // card
      console.log(item.card);
  
      var card = new UI.Card();
      card.title(item.title);
      card.subtitle(item.subtitle);
      card.body(item.card.body);
      card.show();
    }
    
  });
  
  menu.show();
});

main.on('click', 'select', function(e) {
  var wind = new UI.Window();
  var textfield = new UI.Text({
    position: new Vector2(0, 50),
    size: new Vector2(144, 30),
    font: 'gothic-24-bold',
    text: 'Text Anywhere!',
    textAlign: 'center'
  });
  wind.add(textfield);
  wind.show();
});

main.on('click', 'down', function(e) {
  var card = new UI.Card();
  card.title('A Card');
  card.subtitle('Is a Window');
  card.body('The simplest window type in Pebble.js.');
  card.show();
});
