char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>LED Test Page</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
    html {
        height: 100%;
    }

    body {
        background-color: beige;
        height: 100%;
        margin: 0;
        font-family: "Arial";
        display: grid;
        grid-template-rows: 1fr 8fr 1fr;
        user-select: none;
        -moz-user-select: none;
    }

    #wrapper {
        grid-row: 2;
        height: 100%;
        display: grid;
        grid-template-rows: 1fr 1fr;
        grid-row-gap: 10%;
    }

    .text {
        font-style: italic;
        margin: 0;
        text-align: center;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        font-size: 15vw;
        transition: font-size 0.02s ease-in;
    }

    .boxes {
        width: 70%;
        left: 15%;
        border-radius: 10px;
        overflow: hidden;
        text-overflow: ellipsis;
        position: relative;
    }

    #button {
        background-color: darkslategray;
        color: white;
        cursor: pointer;
        transition: background-color 0.3s ease-out;
    }

    #button:hover {
        background-color: forestgreen;
    }

    #button:active > p {
        font-size: 12vw;
    }

    #statusText {
        font-size: 8vw;
    }
</style>
</head>
<body>
    <div id="wrapper">
        <div class="boxes" id="button" onclick="toggle()"><p class="text">TOGGLE</p></div>
        <div class="boxes"><p class="text" id="statusText">...</p></div>
    </div>
    <script>
        function receiveStatus() {
            var getStat = new XMLHttpRequest();
            getStat.open("GET", "/update", true);
            getStat.send();
            getStat.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    document.getElementById("statusText").innerHTML = this.responseText;
                }
            }
            setTimeout(receiveStatus, 500);
        }
        receiveStatus();
        document.addEventListener('DOMContentLoaded', receiveStatus, false);
        
        function toggle() {
            var onCall = new XMLHttpRequest();
            onCall.open("POST", "/toggle", true);
            onCall.send();
        }
    </script>
</body>
</html>
)=====";
