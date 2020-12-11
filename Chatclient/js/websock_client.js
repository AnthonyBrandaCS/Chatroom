var socket = new WebSocket("ws://69.42.2.15:8000/chat");
var name = "";
var chat = "";

socket.addEventListener('open', function(event) {
    document.getElementById("chatbox").innerHTML = "<p>You have connected.<br>Please enter your name to start chatting.<br><br></p>"
    document.getElementById("usermsg").placeholder = "Enter your name...";
});

function submit() {

    var input = document.getElementById("usermsg").value;
    document.getElementById("usermsg").value = "";

    if (input.length > 0) {
        if (name == "") {
            name = input;
            socket.send("\\name " + name);
            document.getElementById("usermsg").placeholder = "";
            document.getElementById("menu").innerHTML = "<p>Welcome, <b id=\"name\">" + name + "</b></p>" + document.getElementById("menu").innerHTML;
            document.getElementById("chatbox").innerHTML = chat;
        } else if (input == "\\clear") {
            document.getElementById("chatbox").innerHTML = "";
        } else if (input == "\\help") {
            document.getElementById("chatbox").innerHTML += "<br>" +
                "<p>\\name <NAME> - Changes your name.</p>" +
                "<p>\\clear - clears the chatbox.</p>" +
                "<br>";
        } else {
            socket.send(input);
        }
    }
}

$('#usermsg').on('keypress', function(event) {
    if (event.key == 'Enter') {
        submit();
    }
});

function exit() {
    document.getElementById("chatbox").innerHTML += "<p>You have disconnected.</p>";
    socket.send("\\exit");
    socket.close();
}

socket.addEventListener('message', function(event) {
    var scroll = false;

    if (document.getElementById("chatbox").scrollTop == document.getElementById("chatbox").scrollHeight - document.getElementById("chatbox").clientHeight) {
        scroll = true;
    }

    if (name == "") {
        chat += "<p>" + event.data + "</p>";
    }
    if (event.data.includes("\\name ")) {
        name = event.data.replace("\\name ", "");
        document.getElementById("name").innerHTML = name;
        document.getElementById("chatbox").innerHTML += "<p>Your name has been changed to " + name + "</p>";
    } else {
        document.getElementById("chatbox").innerHTML += "<p>" + event.data + "</p>";
    }

    if (scroll == true) {
        document.getElementById("chatbox").scrollTop = document.getElementById("chatbox").scrollHeight;
    }
});

window.onbeforeunload = exit;