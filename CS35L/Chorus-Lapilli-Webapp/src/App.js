import { useState } from 'react';

function Square({ value, onSquareClick }){
  return (
    <button className="square" onClick={onSquareClick}>
    {value}
    </button>
  )
}

function Board({ xIsNext, squares, onPlay, currentMove }) {

  let stage2obj = {
    pickedUpYet: false,
    piece: null,
    location: null,
    needToWin: false,
    winningMoves: []
  }
  function handleClick(i){

    if (currentMove >= 6){
      if(stage2obj.pickedUpYet === false)
        stage2pickup(i);
      else{
        stage2place(i);
      }
    }
    else {
      stage1(i);
    }
  }

  function stage1(i){
    if (squares[i] || calculateWinner(squares)){
      return;
    }

    const nextSquares = squares.slice();
    if (xIsNext){
      nextSquares[i] = "X";
    }
    else{
      nextSquares[i] = "O";
    }
    onPlay(nextSquares);
  }
  function stage2pickup(i){
    const objectToPickUp = (xIsNext ? "X" : "O");

    if (squares[i] !== objectToPickUp || calculateWinner(squares) || !PossiblePlays(i, squares)){ //valid square check
      return;
    }

    if (squares[4] === objectToPickUp && i !== 4){ //center square rule
      if (!possibleToWin(i, squares)){
        return;
      }
      else {
        stage2obj.needToWin = true;
        stage2obj.winningMoves = winningMoves(i, squares);
      }
    } 

    stage2obj.pickedUpYet = true;
    stage2obj.piece = squares[i];
    stage2obj.location = i;
  }
  function stage2place(i){
    const piece = stage2obj.piece //X or O
    if ( (!isAdjacent(i, stage2obj.location)) || (squares[i])){ //must click on adjacent, empty square
      return;
    }
    if (stage2obj.needToWin){
      if (!stage2obj.winningMoves.includes(i)){ //we have a winning move, you have to play it by rule
        return;
      }
    }
    const nextSquares = squares.slice();
    nextSquares[i] = piece;
    nextSquares[stage2obj.location] = null;
    onPlay(nextSquares);
  }

  const winner = calculateWinner(squares);
  let status;
  if (winner) {
    status = "Winner: " + winner;
  } else {
    status = "Next player: " + (xIsNext ? "X" : "O");
  }

  return ( 
  <>
    <div className="status">{status}</div>
    <div className="board-row">
      <Square value={squares[0]} onSquareClick={() => handleClick(0)}/>
      <Square value={squares[1]} onSquareClick={() => handleClick(1)}/>
      <Square value={squares[2]} onSquareClick={() => handleClick(2)}/>
    </div>
    <div className="board-row">
      <Square value={squares[3]} onSquareClick={() => handleClick(3)}/>
      <Square value={squares[4]} onSquareClick={() => handleClick(4)}/>
      <Square value={squares[5]} onSquareClick={() => handleClick(5)}/>
    </div>
    <div className="board-row">
      <Square value={squares[6]} onSquareClick={() => handleClick(6)}/>
      <Square value={squares[7]} onSquareClick={() => handleClick(7)}/>
      <Square value={squares[8]} onSquareClick={() => handleClick(8)}/>
    </div>
  </>
  )
}

export default function Game() {
  const [currentMove, setCurrentMove] = useState(0);
  const xIsNext = (currentMove % 2 === 0);
  const [currentSquares, setCurrentSquares] = useState(Array(9).fill(null));

  function handlePlay(nextSquares) {
    setCurrentSquares(nextSquares);
    setCurrentMove(currentMove+1);
  }

  return (
    <div className="game">
      <div className="game-board">
        <Board xIsNext={xIsNext} squares={currentSquares} onPlay={handlePlay} currentMove={currentMove}/>
      </div>
    </div>
  );
}

function isAdjacent(a, b){
  const adjacencyMatrix = [
    [false, true, false, true, true, false, false, false, false],
    [true, false, true, true, true, true, false, false, false],
    [false, true, false, false, true, true, false, false, false],
    [true, true, false, false, true, false, true, true, false],
    [true, true, true, true, false, true, true, true, true],
    [false, true, true, false, true, false, false, true, true],
    [false, false, false, true, true, false, false, true, false],
    [false, false, false, true, true, true, true, false, true],
    [false, false, false, false, true, true, false, true, false]
  ];
  return adjacencyMatrix[a][b];
}

function calculateWinner(squares) {
  const lines = [
    [0, 1, 2],
    [3, 4, 5],
    [6, 7, 8],
    [0, 3, 6],
    [1, 4, 7],
    [2, 5, 8],
    [0, 4, 8],
    [2, 4, 6]
  ];
  for (let i = 0; i < lines.length; i++) {
    const [a, b, c] = lines[i];
    if (squares[a] && squares[a] === squares[b] && squares[a] === squares[c]) {
      return squares[a];
    }
  }
  return null;
}

function PossiblePlays(i, squares){
  for(let j = 0; j < 9; j++){
    if( isAdjacent(i,j) && (!squares[j]) ){
      return true;
    }
  }
  return false;
}

function winningMoves(i, squares){
  let winningArray = [];

  for(let j = 0; j < 9; j++){
    let tempSquares = squares.slice();
    if( isAdjacent(i,j) && (!tempSquares[j]) ){ //can move from i to j
      tempSquares[j] = tempSquares[i];
      tempSquares[i] = null;
      if(calculateWinner(tempSquares)){
        winningArray.push(j);
      }
    }
  }
  return winningArray;
}
function possibleToWin(i, squares){
  let winningArray = winningMoves(i, squares);
  if (winningArray.length === 0){
    return false;
  }
  else{
    return true;
  }
}