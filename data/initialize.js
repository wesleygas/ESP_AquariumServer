$(document).ready(function(){
    $('.tabs').tabs();
});

$(document).ready(function(){
    $('select').formSelect();
});

function makeArr(start, stop, cardinality){
    var arr = [];
    var step = (stop - start)/(cardinality - 1)
    for(var i = 0; i< cardinality; i++){
        arr.push(start + (step*i))
    }
    return arr
}

function convertToHumanHour(decHour){
    intPart = Math.floor(decHour)
    decPart = decHour - intPart
    return [intPart,decPart*60]
}

function calcMean(start, end){
    diff = end - start;
    if(diff < 0){
        console.error("Diff",diff,"eh negativo");
    }
    return [diff, start+(diff/2)];
}

function calcC(diff, a, limit){
    return -(diff**2)/(8*Math.log(limit/a))
}

function calcGauss(x){
    var exponent = ((x-mean)**2)/(2*stdDev);
    if(start < x && x < end){
        return [x,max*Math.exp(-exponent)];
    }else{
        return [x, 0];
    }
}

function updateGauss(params){
    window.max = params.max;
    window.start = params.start;
    window.end = params.end;
    var ret = calcMean(params.start, params.end);
    diff = ret[0];
    mean = ret[1];
    stdDev = calcC(diff, max, params.limit);
    return t.map(calcGauss)
}