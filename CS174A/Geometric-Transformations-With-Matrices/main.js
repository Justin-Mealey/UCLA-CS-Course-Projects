import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';


const scene = new THREE.Scene();

//THREE.PerspectiveCamera( fov angle, aspect ratio, near depth, far depth );
const camera = new THREE.PerspectiveCamera( 75, window.innerWidth / window.innerHeight, 0.1, 1000 );

const renderer = new THREE.WebGLRenderer();
renderer.setSize( window.innerWidth, window.innerHeight );
document.body.appendChild( renderer.domElement );

const controls = new OrbitControls(camera, renderer.domElement);
camera.position.set(0, 5, 10);
controls.target.set(0, 5, 0);

// Rendering 3D axis
const createAxisLine = (color, start, end) => {
    const geometry = new THREE.BufferGeometry().setFromPoints([start, end]);
    const material = new THREE.LineBasicMaterial({ color: color });
    return new THREE.Line(geometry, material);
};
const xAxis = createAxisLine(0xff0000, new THREE.Vector3(0, 0, 0), new THREE.Vector3(3, 0, 0)); // Red
const yAxis = createAxisLine(0x00ff00, new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 3, 0)); // Green
const zAxis = createAxisLine(0x0000ff, new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 3)); // Blue
scene.add(xAxis);
scene.add(yAxis);
scene.add(zAxis);


// ***** Assignment 2 *****
// Setting up the lights
const pointLight = new THREE.PointLight(0xffffff, 100, 100);
pointLight.position.set(5, 5, 5); // Position the light
scene.add(pointLight);

const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
directionalLight.position.set(0.5, .0, 1.0).normalize();
scene.add(directionalLight);

const ambientLight = new THREE.AmbientLight(0x505050);  // Soft white light
scene.add(ambientLight);

const phong_material = new THREE.MeshPhongMaterial({
    color: 0x00ff00, // Green color
    shininess: 100   // Shininess of the material
});


// Start here.

const l = 0.5
const positions = new Float32Array([
    // Front face
    -l, -l,  l, // 0
     l, -l,  l, // 1
     l,  l,  l, // 2
    -l,  l,  l, // 3

    // Left face
    -l, -l, -l, // 4
    -l, -l,  l, // 5
    -l,  l,  l, // 6 
    -l,  l, -l, // 7
  
    // Top face
    -l,  l,  l, // 8
     l,  l,  l, // 9
     l,  l, -l, // 10
    -l,  l, -l, // 11

    // Bottom face
    -l, -l, -l, // 12
     l, -l, -l, // 13
     l, -l,  l, // 14
    -l, -l,  l, // 15

    // Right face
     l, -l,  l, // 16
     l, -l, -l, // 17
     l,  l, -l, // 18
     l,  l,  l, // 19

    // Back face
     l, -l, -l, // 20
    -l, -l, -l, // 21
    -l,  l, -l, // 22
     l,  l, -l  // 23
  ]);
  
  const indices = [
    // Front face
    0, 1, 2,
    0, 2, 3,
  
    // Left face
    4, 5, 6,
    4, 6, 7,
  
    // Top face
    8, 9, 10,
    8, 10, 11,

    // Bottom face
    12, 13, 14,
    12, 14, 15,

    // Right face
    16, 17, 18,
    16, 18, 19,

    // Back face
    20, 21, 22,
    20, 22, 23
  ];
  
  // Compute normals
  const normals = new Float32Array([
    // Front face
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
  
    // Left face
    -1, 0, 0,
    -1, 0, 0,
    -1, 0, 0,
    -1, 0, 0,
  
    // Top face
    0, 1, 0,
    0, 1, 0,
    0, 1, 0,
    0, 1, 0,

    // Bottom face
    0, -1, 0,
    0, -1, 0,
    0, -1, 0,
    0, -1, 0,

    // Right face
    1, 0, 0,
    1, 0, 0,
    1, 0, 0,
    1, 0, 0,

    // Back face
    0, 0, -1,
    0, 0, -1,
    0, 0, -1,
    0, 0, -1
  ]);

const custom_cube_geometry = new THREE.BufferGeometry();
custom_cube_geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
custom_cube_geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
custom_cube_geometry.setIndex(new THREE.BufferAttribute(new Uint16Array(indices), 1));

// TODO: Implement wireframe geometry

// WIREFRAME
const wireframe_vertices = new Float32Array([
  // Front face
      -l, -l, l,
      l, -l, l,

      l, -l, l,
      l, l, l,

      l, l, l,
      -l, l, l,

      -l, l, l,
      -l, -l, l,

  // Top face
      -l, l, -l,
      -l, l, l,

      -l, l, l,
      l, l, l,

      l, l, l,
      l, l, -l,

      l, l, -l,
      -l, l, -l,
      

  // Bottom Face
      -l, -l, -l, 
       l, -l, -l, 

       l, -l, -l, 
       l, -l,  l, 

       l, -l,  l, 
      -l, -l,  l, 

      -l, -l,  l, 
      -l, -l, -l, 

  // Right Face
      l, -l,  l, 
      l, -l, -l, 

      l, -l, -l, 
      l,  l, -l, 

      l,  l, -l, 
      l,  l,  l, 

      l,  l,  l, 
      l, -l,  l,

  // Left Face
      -l, -l, -l, 
      -l, -l,  l, 

      -l, -l,  l, 
      -l,  l,  l, 

      -l,  l,  l, 
      -l,  l, -l, 

      -l,  l, -l, 
      -l, -l, -l, 

  // Back Face
      l, -l, -l, 
      -l, -l, -l, 

      -l, -l, -l, 
      -l,  l, -l, 

      -l,  l, -l, 
      l,  l, -l,  

      l,  l, -l, 
      l, -l, -l
]);

const wireframe_greometry = new THREE.BufferGeometry();
wireframe_greometry.setAttribute( 'position', new THREE.BufferAttribute( wireframe_vertices, 3 ) );
// WIREFRAME END

//TRANSFORMATIONS
function translationMatrix(tx, ty, tz) {
	return new THREE.Matrix4().set(
		1, 0, 0, tx,
		0, 1, 0, ty,
		0, 0, 1, tz,
		0, 0, 0, 1
	);
}
// TODO: Implement the other transformation functions.

// OTHER TRANSFORMATION FUNCTIONS
function rotationMatrixZ(theta) {
	return new THREE.Matrix4().set(
        Math.cos(theta), -1 * Math.sin(theta), 0, 0,
        Math.sin(theta), Math.cos(theta), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
	);
}
function scalingMatrix(sx, sy, sz) {
  return new THREE.Matrix4().set(
    sx, 0,  0,  0,  //x
    0,  sy, 0,  0,  //y
    0,  0,  sz, 0,  //z
    0,  0,  0,  1   //1
  );
}
// OTHER TRANSFORMATION FUNCTIONS END
//TRANSFORMATIONS END

//INIT CUBES
let cubes_m = [];//mesh array
let cubes_w = [];//wireframe array
for (let i = 0; i < 7; i++) {
  //mesh cubes, initially visible
	let meshCube = new THREE.Mesh(custom_cube_geometry, phong_material);
	meshCube.matrixAutoUpdate = false;
	cubes_m.push(meshCube);
	scene.add(meshCube);

  //wireframe cubes, initially invisible
  let line = new THREE.LineSegments( wireframe_greometry );
  let wireframeCube = line
  wireframeCube.matrixAutoUpdate = false
  wireframeCube.visible = false
  cubes_w.push(wireframeCube)
  scene.add(wireframeCube);
}
//INIT CUBES END

// TODO: Transform cubes

//TRANSFORM CUBES

//RESIZE CUBES
for (let i = 0; i < cubes_m.length; i++) {
  cubes_m[i].matrix.multiply(scalingMatrix(1.5, 1.5, 1.5))
  cubes_w[i].matrix.multiply(scalingMatrix(1.5, 1.5, 1.5))
}
//RESIZE CUBES END

//EXERCISE 2
// let T1 = translationMatrix(.5, .5, 0)
// let twenty = (20 * Math.PI)/180
// let R = rotationMatrixZ(twenty)
// let T2 = translationMatrix(-.5, .5, 0)

// let M = new THREE.Matrix4()
// //rotate and stack
// for (let i = 1; i < cubes.length; i++) {
//   M.multiply(T2).multiply(R).multiply(T1)
//   cubes[i].matrix.multiply(M)
// }
//EXERCISE 2 END

//TRANSFORM CUBES END

//ANIMATE CUBES
let animation_time = 0;
let delta_animation_time;
let rotation_angle;
const clock = new THREE.Clock();
let originalMatrix = cubes_m[0].matrix

let still = false;

function animate() {
  
	renderer.render( scene, camera );
  controls.update();
  
  // TODO
  // Animate the cube
  
    delta_animation_time = clock.getDelta();
    animation_time += delta_animation_time; 
  if (!still) {
    let omega = (2 * Math.PI)/3 //w = 2pi/T
    rotation_angle = 10 + (10 * Math.sin( (omega * animation_time) - (Math.PI/2)))
    let rot_in_radians = (rotation_angle * Math.PI)/180

    const rotation = rotationMatrixZ(rot_in_radians);
    let T1 = translationMatrix(.5, .5, 0)
    let T2 = translationMatrix(-.5, .5, 0)

    let M = new THREE.Matrix4() //for transforming mesh cubes
    let M2 = new THREE.Matrix4() //for transforming wireframe cubes
    // Apply transformations
    for (let i = 1; i < cubes_m.length; i++) {
      cubes_m[i].matrix.copy(originalMatrix)
      M.multiply(T2).multiply(rotation).multiply(T1)
      cubes_m[i].matrix.multiply(M)

      cubes_w[i].matrix.copy(originalMatrix)
      M2.multiply(T2).multiply(rotation).multiply(T1)
      cubes_w[i].matrix.multiply(M2)
    }
  }
}
renderer.setAnimationLoop( animate );
//ANIMATE CUBES END

// TODO: Add event listener
//HANDLE KEYS
window.addEventListener('keydown', onKeyPress); // onKeyPress is called each time a key is pressed
// Function to handle keypress
function onKeyPress(event) {
    switch (event.key) {
        case 's': // Note we only do this if s is pressed.
            still = !still;
            //set max angle
            const max_angle = (20 * Math.PI)/180
            const rotation = rotationMatrixZ(max_angle);
            let T1 = translationMatrix(.5, .5, 0)
            let T2 = translationMatrix(-.5, .5, 0)

            let M = new THREE.Matrix4() //for transforming mesh cubes
            let M2 = new THREE.Matrix4() //for transforming wireframe cubes
            // Apply transformations
            for (let i = 1; i < cubes_m.length; i++) {
              cubes_m[i].matrix.copy(originalMatrix)
              M.multiply(T2).multiply(rotation).multiply(T1)
              cubes_m[i].matrix.multiply(M)

              cubes_w[i].matrix.copy(originalMatrix)
              M2.multiply(T2).multiply(rotation).multiply(T1)
              cubes_w[i].matrix.multiply(M2)
            }
            break;
        case 'w':
          for (let i = 0; i < 7; i++) {
            cubes_m[i].visible = !cubes_m[i].visible
            cubes_w[i].visible = !cubes_w[i].visible
          }
          break;
        default:
            console.log(`Key ${event.key} pressed`);
    }
}
//HANDLE KEYS END
