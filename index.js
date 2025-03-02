document.addEventListener("DOMContentLoaded", async (event) => {
    event.preventDefault();
    getSlot();
    setInterval(getSlot, 3000);
});

async function getSlot() {
    let amount = 4;
    const data = await getAllSlotInformation();

    const boxItem = document.querySelector("#box-item");
    // Xóa nội dung cũ trước khi thêm mới
    boxItem.innerHTML = "";

    for (let key in data) {
        // Tạo div item
        let div = document.createElement("div");
        div.classList.add("item");

        // Tạo 2 thẻ <p>
        const p1 = document.createElement("p");
        p1.textContent = key;
        const p2 = document.createElement("p");
        if (data[key]) {
            p2.textContent = "Empty";
            p2.classList.add("neon-green");
        } else {
            p2.textContent = "Full";
            p2.classList.add("neon-red");
            amount--;
        }

        // Nối vào div item
        div.appendChild(p1);
        div.appendChild(p2);

        // Nối vào #box-item
        boxItem.appendChild(div);
        document.querySelector("#amount").innerHTML = amount;
    }
};

async function getAllSlotInformation() {
    const respond = await fetch("https://script.google.com/macros/s/AKfycby4MD6yYUN_b7oaMwIm-okSs4OAhjRrZJfS4UI62bPycZ-qdCrCuOs_3ru2hrroEzT1Bg/exec");
    const result = await respond.json();
    console.log(result.data);
    return result.data;
}