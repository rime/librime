const initials = 'bpmfdtnlgkhjqxzcsryw';
const len = Number(process.argv[2]);
let t = [];
for (let i = 0; i < len; ++i) {
    t.push(initials.charAt(Math.floor(Math.random() * initials.length)));
}
console.log(t.join(''));
