document.addEventListener("DOMContentLoaded", function () {
  var status = document.getElementById("status");
  status.textContent = "JS loaded";

  var btn = document.getElementById("btn");
  btn.addEventListener("click", function () {
    status.textContent = "Fetching / ...";
    fetch("/")
      .then(function (res) {
        status.textContent = "Fetched / — status: " + res.status;
      })
      .catch(function (err) {
        status.textContent = "Fetch error: " + err;
      });
  });
});
