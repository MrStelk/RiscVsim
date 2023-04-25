function CycleSelect(id){
    var dic = {{ pipeline|tojson|safe }};
    var list = dic[id];
    //const x = document.getElementById("instruction_details");
    //const y = x.getElementsByTagName("p");
    var cynum = document.getElementById("instruction_details_cynum");
	cynum.innerHTML = "id";	
}