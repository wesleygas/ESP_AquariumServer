//ESSES VALORES INICIAIS VAO SER OBTIDOS A PARTIR DOS VALORES ATUAIS DO MCU
var minutesResolution = 10;

t = makeArr(0,24,24*minutesResolution);

lightData = {
    ww: {
        number: 0,
        color: "#d6b85c",
        start: 6,
        end: 18,
        max: 20,
        limit: 1
    },
    cw: {
        number: 1,
        color: "#abb8de",
        start: 10,
        end: 20,
        max: 30,
        limit: 1
    }
};

var xmlHttp = new XMLHttpRequest();
xmlHttp.open( "GET", "/lightdata", false ); // false for synchronous request
xmlHttp.send( null );
original_lightData = JSON.parse(xmlHttp.responseText);



for(var color in original_lightData){
    lightData[color].start = original_lightData[color].start;
    lightData[color].end = original_lightData[color].end;
    lightData[color].max = original_lightData[color].max;
    lightData[color].limit = original_lightData[color].limit;
}

$('#pwr-text').text("Power: "+ lightData.ww.max + "%");
$('#flr-text').text("Floor: "+ lightData.ww.limit);

var timeNow = 10.5;

var xmlHttp = new XMLHttpRequest();
xmlHttp.open( "GET", "/timeval", false ); // false for synchronous request
xmlHttp.send( null );
timeNow = parseFloat(xmlHttp.responseText);


var activeColor = "ww";

ww_data = updateGauss(lightData.ww);
cw_data = updateGauss(lightData.cw);

var colors = Highcharts.getOptions().colors;

chartSeries = {
    whites: [
        {
            name: "warm_white",
            data: ww_data,
            color: '#d6b85cb0',
            zoneAxis: 'x',
            zones: [{
                value: timeNow,
                color: '#d6b85cb0'
            }, {
                color: '#d6b85c50'
            }]
        },{
            name:"cold white",
            data: cw_data,
            color: '#abb8deb0',
            zoneAxis: 'x',
            zones: [{
                value: timeNow,
                color: '#abb8deb0'
            }, {
                color: '#abb8de50'
            }]
        }
    ],
    RGB: []
};

var chart = Highcharts.chart('lighting_preview', {
    chart: {
        type: 'areaspline'
    },
    credits: {
        enabled: false
    },
    tooltip:{
        formatter: function(){
            hora = parseFloat(this.x);
            minutos = Math.floor((hora%1)* 60);
            hora = Math.floor(hora);
            strRet = "<b>Potencia:</b> "+ Math.floor(this.y);
            strRet += "%<br><b>Hora:</b> "+ hora + "h" + minutos;
            return  strRet;
        }
    },
    title: {
        text: "luz",
        align: 'center'
    },
    xAxis:{
        tickInterval:1
    },
    yAxis: {
        max: 100,
        title:{
            text:'power(%)'
        },
        labels:{
            format: '{value}%'
        }
    },
    series: chartSeries['whites']
});

// // To disable
// slider.setAttribute('disabled', true);

// // To re-enable
// slider.removeAttribute('disabled');



ttFormatter = {
    to: function(x){
            hora = parseFloat(x);
            minutos = Math.floor((hora%1)* 60);
            hora = Math.floor(hora);
            return "" + hora + "h" + minutos;
        },
    from: function(x){
        tol = x.split(':');
        return parseInt(tol[0])+parseInt(tol[1]/60);
    }
}

var hourSlider = document.getElementById('hour-slider');
noUiSlider.create(hourSlider, {
    start: [lightData[activeColor].start, lightData[activeColor].end],
    connect: true,
    tooltips: [ttFormatter, ttFormatter],
    pips: {
        mode: 'values',
        values: [0,6,12,18,24],
        density: 4,
        stepped: true
    },
    margin: 1,
    step: 1/60,
    orientation: 'horizontal', // 'horizontal' or 'vertical'
    range: {
        'min': 0,
        'max': 24
    }}
);

hourSlider.querySelectorAll('.noUi-connect')[0].classList.add('whites-slider');

hourSlider.noUiSlider.on('change', function(values, handle){
    lightData[activeColor].start = parseFloat(values[0]);
    lightData[activeColor].end = parseFloat(values[1]);
    chart.series[lightData[activeColor].number].update({
        data: updateGauss(lightData[activeColor]),
        zones: [{
            value: timeNow,
            color: lightData[activeColor].color+'b0'
        }, {
            color: lightData[activeColor].color+'50'
        }]
    }, true)
    sendParameters(false);
});

var powerSlider = document.getElementById('power-slider');
noUiSlider.create(powerSlider, {
    start: lightData[activeColor].max,
    connect: [true, false],
    pips: {
        mode: 'count',
        values: 11,
        density: 4,
        stepped: true
    },
    step: 1/10,
    orientation: 'horizontal', // 'horizontal' or 'vertical'
    range: {
        'min': 0,
        'max': 100
    }}
);

powerSlider.querySelectorAll('.noUi-connect')[0].classList.add('whites-slider');

powerSlider.noUiSlider.on('change', function(values, handle){
    lightData[activeColor].max = parseFloat(values[0]);
    $('#pwr-text').text("Power: "+ values[0]+ "%")
    chart.series[lightData[activeColor].number].update({
        data: updateGauss(lightData[activeColor])}, true)
    sendParameters(false);
});

powerSlider.noUiSlider.on('update', function(values, handle){
    $('#pwr-text').text("Power: "+ values[0]+ "%");
});

var limitSlider = document.getElementById('limit-slider');
noUiSlider.create(limitSlider, {
    start: lightData[activeColor].limit,
    connect: [true, false],
    pips: {
        mode: 'count',
        values: 11,
        density: 4,
        stepped: true
    },
    step: 1/20,
    orientation: 'horizontal', // 'horizontal' or 'vertical'
    range: {
        'min': 0.01,
        'max': 20
    }}
);


limitSlider.querySelectorAll('.noUi-connect')[0].classList.add('whites-slider');

limitSlider.noUiSlider.on('change', function(values, handle){
    lightData[activeColor].limit = parseFloat(values[0]);
    $('#flr-text').text("Floor: "+ values[0])
    chart.series[lightData[activeColor].number].update({
        data: updateGauss(lightData[activeColor])}, true)
    sendParameters(false);
    
});
limitSlider.noUiSlider.on('update', function(values, handle){
    $('#flr-text').text("Floor: "+ values[0])
});

$("select.white-color").change(function(){
    activeColor = $(this).children('option:selected').val();
    $('.whites-slider').css("background", lightData[activeColor].color)
    hourSlider.noUiSlider.set([lightData[activeColor].start,lightData[activeColor].end])
    powerSlider.noUiSlider.set(lightData[activeColor].max);
    limitSlider.noUiSlider.set(lightData[activeColor].limit);
});

$('a[href="#RGB"]').click(function(){
    //Change the chart
}); 

$('a[href="#whites"]').click(function(){
    //Change the chart
}); 

function sendParameters(save){
    for(var color in original_lightData){
        original_lightData[color].start = lightData[color].start;
        original_lightData[color].end = lightData[color].end;
        original_lightData[color].max = lightData[color].max;
        original_lightData[color].limit = lightData[color].limit;
    }
    var xhr = new XMLHttpRequest();
    xhr.withCredentials = true;
    url = "/lightdata"
    if(save) url+= "?save=1"
    xhr.open("POST", url, false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(original_lightData));
}

$('#restore').click(function(){
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", "/lightdata", false ); // false for synchronous request
    xmlHttp.send( null );
    original_lightData = JSON.parse(xmlHttp.responseText);

    for(var color in original_lightData){
        lightData[color].start = original_lightData[color].start;
        lightData[color].end = original_lightData[color].end;
        lightData[color].max = original_lightData[color].max;
        lightData[color].limit = original_lightData[color].limit;
        chart.series[lightData[color].number].update({
            data: updateGauss(lightData[color])}, true)
    }
    hourSlider.noUiSlider.set([lightData[activeColor].start,lightData[activeColor].end])
    powerSlider.noUiSlider.set(lightData[activeColor].max);
    limitSlider.noUiSlider.set(lightData[activeColor].limit);
    sendParameters(false);
}); 

$('#send').click(function(){
    sendParameters(true);
    alert("Success!");
}); 
